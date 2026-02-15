/**
 * main.c - IP over Sound 程序入口与线程调度
 *
 * 流程：
 * 1. 打开 TUN、初始化音频、创建调制/解调器
 * 2. 启动 TX 线程：TUN 读 -> 封装 -> 调制 -> 音频写
 * 3. 启动 RX 线程：音频读 -> 解调 -> 找帧/解封装 -> TUN 写
 * 4. 主线程等待 Ctrl+C 或信号后清理退出
 */

#include "common.h"
#include "tun_dev.h"
#include "audio_dev.h"
#include "modem.h"
#include "protocol.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

/* 全局运行标志：收到 SIGINT 时置 0，各线程退出 */
static volatile int g_running = 1;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

/**
 * 将一帧字节转为比特流（每字节 8 比特，高位在前）
 * 用于调制前把 frame 转为 modem_tx_modulate 需要的 bits 数组
 */
static void frame_to_bits(const uint8_t *frame, int frame_len, uint8_t *bits_out, int *nbits_out)
{
    int i, b;
    *nbits_out = frame_len * 8;
    for (i = 0; i < frame_len; i++) {
        for (b = 7; b >= 0; b--) {
            int bit_idx = i * 8 + (7 - b);
            int byte_idx = bit_idx / 8;
            int bit_in_byte = 7 - (bit_idx % 8);
            if (frame[i] & (1 << b))
                bits_out[byte_idx] |= (1 << bit_in_byte);
            else
                bits_out[byte_idx] &= ~(1 << bit_in_byte);
        }
    }
}

/**
 * TX 线程：从 TUN 读取 IP 包 -> 封装成帧 -> 调制 -> 写入扬声器
 */
static void *tx_thread_func(void *arg)
{
    int tun_fd = *(int *)arg;
    uint8_t *ip_buf;
    uint8_t *frame_buf;
    uint8_t *bits_buf;
    sample_t *samples_buf;
    int frame_len, nbits, nsamples, i, written;
    modem_tx_handle_t mod_tx;
    audio_handle_t audio = NULL;  /* 由 main 传入更佳，此处简化用全局或参数 */
    const size_t max_samples = (size_t)(MAX_FRAME_LEN * 8) * SAMPLES_PER_BIT;

    ip_buf    = (uint8_t *)malloc(MAX_FRAME_PAYLOAD);
    frame_buf = (uint8_t *)malloc(MAX_FRAME_LEN);
    bits_buf  = (uint8_t *)malloc(MAX_FRAME_LEN + 1);  /* 按字节存比特 */
    samples_buf = (sample_t *)malloc(max_samples * sizeof(sample_t));
    mod_tx    = modem_tx_create();

    if (!ip_buf || !frame_buf || !bits_buf || !samples_buf || !mod_tx) {
        fprintf(stderr, "tx_thread: alloc or modem_tx_create failed\n");
        if (ip_buf) free(ip_buf);
        if (frame_buf) free(frame_buf);
        if (bits_buf) free(bits_buf);
        if (samples_buf) free(samples_buf);
        if (mod_tx) modem_tx_destroy(mod_tx);
        return NULL;
    }

    /* 从 main 传过来的参数里拿到 audio 和 tun_fd；这里用全局/静态简化，见下方 */
    extern audio_handle_t g_audio_handle;
    audio = g_audio_handle;

    while (g_running && audio) {
        int n = tun_read(tun_fd, ip_buf, MAX_FRAME_PAYLOAD);
        if (n <= 0) continue;

        frame_len = protocol_encapsulate(ip_buf, n, frame_buf);
        if (frame_len <= 0) continue;

        nbits = frame_len * 8;
        frame_to_bits(frame_buf, frame_len, bits_buf, &nbits);
        nsamples = modem_tx_modulate(mod_tx, bits_buf, nbits, samples_buf);
        if (nsamples <= 0) continue;

        /* 按块写入声卡，避免一次写太多 */
        for (i = 0; i < nsamples && g_running; i += AUDIO_FRAMES_PER_BUFFER) {
            written = (nsamples - i) < AUDIO_FRAMES_PER_BUFFER ? (nsamples - i) : AUDIO_FRAMES_PER_BUFFER;
            if (audio_write(audio, samples_buf + i, written) != 0)
                break;
        }
    }

    free(ip_buf);
    free(frame_buf);
    free(bits_buf);
    free(samples_buf);
    modem_tx_destroy(mod_tx);
    return NULL;
}

/** 接收端比特缓冲：需累积多块解调结果才能凑齐一帧 */
#define RX_BIT_BUF_BYTES  (MAX_FRAME_LEN * 4)
#define RX_BIT_BUF_BITS   (RX_BIT_BUF_BYTES * 8)

/** 从比特流中取出一段转为字节（用于将找到的帧从 bits 转为 frame_buf） */
static void bits_to_bytes(const uint8_t *bits, int bit_start, int nbytes, uint8_t *bytes_out)
{
    int by, b, bi;
    for (by = 0; by < nbytes; by++) {
        bytes_out[by] = 0;
        for (b = 0; b < 8; b++) {
            bi = bit_start + by * 8 + b;
            if (bits[bi / 8] & (1 << (7 - bi % 8)))
                bytes_out[by] |= (1 << (7 - b));
        }
    }
}

/** 将 src 的 nbits 比特追加到 dest，从 dest_bit_count 位置开始；返回新的 dest 总比特数 */
static int bits_append(uint8_t *dest, int dest_bit_count, const uint8_t *src, int nbits)
{
    int i;
    for (i = 0; i < nbits; i++) {
        int di = dest_bit_count + i;
        int si_byte = i / 8, si_bit = 7 - (i % 8);
        if (src[si_byte] & (1 << si_bit))
            dest[di / 8] |= (1 << (7 - di % 8));
        else
            dest[di / 8] &= ~(1 << (7 - di % 8));
    }
    return dest_bit_count + nbits;
}

/** 从 buf 中移除 [from_bit, from_bit+n_bits) 的比特，将后面的数据前移 */
static void bits_remove(uint8_t *buf, int *bit_count_inout, int from_bit, int n_bits)
{
    int new_count = *bit_count_inout - n_bits;
    int i;
    if (new_count <= 0) {
        *bit_count_inout = 0;
        return;
    }
    for (i = 0; i < new_count; i++) {
        int src_i = from_bit + n_bits + i;
        int dst_i = i;
        int bit_val = (buf[src_i / 8] & (1 << (7 - src_i % 8))) ? 1 : 0;
        if (bit_val)
            buf[dst_i / 8] |= (1 << (7 - dst_i % 8));
        else
            buf[dst_i / 8] &= ~(1 << (7 - dst_i % 8));
    }
    *bit_count_inout = new_count;
}

/**
 * RX 线程：从麦克风读采样 -> 解调成比特 -> 累积 -> 找同步 -> 解封装 -> 写 TUN
 */
static void *rx_thread_func(void *arg)
{
    int tun_fd = *(int *)arg;
    sample_t *audio_buf;
    uint8_t *demod_buf;   /* 本次解调得到的一小块比特 */
    uint8_t *rx_bits_buf; /* 累积的比特流 */
    uint8_t *frame_buf;
    uint8_t *payload_buf;
    int rx_bit_count = 0; /* 当前累积的有效比特数 */
    int nread, nbits, sync_pos, payload_len;
    int frame_byte_len, frame_len_bits;
    modem_rx_handle_t mod_rx;
    audio_handle_t audio = NULL;

    extern audio_handle_t g_audio_handle;
    audio = g_audio_handle;

    audio_buf   = (sample_t *)malloc(AUDIO_FRAMES_PER_BUFFER * sizeof(sample_t));
    demod_buf   = (uint8_t *)malloc(RX_BIT_BUF_BYTES);
    rx_bits_buf = (uint8_t *)malloc(RX_BIT_BUF_BYTES);
    frame_buf   = (uint8_t *)malloc(MAX_FRAME_LEN);
    payload_buf = (uint8_t *)malloc(MAX_FRAME_PAYLOAD);
    mod_rx      = modem_rx_create();

    if (!audio_buf || !demod_buf || !rx_bits_buf || !frame_buf || !payload_buf || !mod_rx) {
        fprintf(stderr, "rx_thread: alloc or modem_rx_create failed\n");
        if (audio_buf) free(audio_buf);
        if (demod_buf) free(demod_buf);
        if (rx_bits_buf) free(rx_bits_buf);
        if (frame_buf) free(frame_buf);
        if (payload_buf) free(payload_buf);
        if (mod_rx) modem_rx_destroy(mod_rx);
        return NULL;
    }

    while (g_running && audio) {
        nread = audio_read(audio, audio_buf, AUDIO_FRAMES_PER_BUFFER);
        if (nread <= 0) continue;

        memset(demod_buf, 0, RX_BIT_BUF_BYTES);
        nbits = modem_rx_demodulate(mod_rx, audio_buf, nread, demod_buf, RX_BIT_BUF_BITS);
        if (nbits <= 0) continue;

        /* 追加到累积缓冲区 */
        if (rx_bit_count + nbits > RX_BIT_BUF_BITS) {
            /* 缓冲区满，丢弃前半部分以腾出空间 */
            bits_remove(rx_bits_buf, &rx_bit_count, 0, rx_bit_count / 2);
        }
        rx_bit_count = bits_append(rx_bits_buf, rx_bit_count, demod_buf, nbits);

        /* 在累积的比特流中找同步 */
        sync_pos = protocol_find_sync(rx_bits_buf, rx_bit_count);
        if (sync_pos < 0) continue;

        /* 需要至少 FRAME_HEADER_LEN 才能读长度字段 */
        if (sync_pos + FRAME_HEADER_LEN * 8 > rx_bit_count) continue;

        bits_to_bytes(rx_bits_buf, sync_pos, FRAME_HEADER_LEN, frame_buf);
        frame_byte_len = (frame_buf[SYNC_LEN] << 8) | frame_buf[SYNC_LEN + 1];
        if (frame_byte_len <= 0 || frame_byte_len > MAX_FRAME_PAYLOAD) {
            /* 非法长度，丢弃同步字前的比特避免死锁 */
            bits_remove(rx_bits_buf, &rx_bit_count, 0, sync_pos + SYNC_LEN * 8);
            continue;
        }
        frame_len_bits = (FRAME_HEADER_LEN + frame_byte_len + CRC_BYTES) * 8;
        if (sync_pos + frame_len_bits > rx_bit_count) continue;

        bits_to_bytes(rx_bits_buf, sync_pos, FRAME_HEADER_LEN + frame_byte_len + CRC_BYTES, frame_buf);
        payload_len = protocol_decapsulate(frame_buf, FRAME_HEADER_LEN + frame_byte_len + CRC_BYTES,
                                           payload_buf, MAX_FRAME_PAYLOAD);
        if (payload_len > 0)
            tun_write(tun_fd, payload_buf, payload_len);

        /* 消费掉这一帧对应的比特 */
        bits_remove(rx_bits_buf, &rx_bit_count, sync_pos, frame_len_bits);
    }

    free(audio_buf);
    free(demod_buf);
    free(rx_bits_buf);
    free(frame_buf);
    free(payload_buf);
    modem_rx_destroy(mod_rx);
    return NULL;
}

/* 全局音频句柄，供 TX/RX 线程使用（也可用参数传递） */
audio_handle_t g_audio_handle = NULL;

int main(int argc, char *argv[])
{
    int tun_fd;
    pthread_t tx_tid, rx_tid;
    const char *tun_name = TUN_DEV_NAME;

    if (argc >= 2)
        tun_name = argv[1];

    signal(SIGINT, signal_handler);

    printf("IP over Sound: opening TUN %s, initializing audio...\n", tun_name);
    tun_fd = tun_open(tun_name);
    if (tun_fd < 0) {
        fprintf(stderr, "Failed to open TUN. Try: sudo ./ipo_sound\n");
        return 1;
    }

    g_audio_handle = audio_init();
    if (!g_audio_handle) {
        fprintf(stderr, "Failed to init audio (PortAudio).\n");
        tun_close(tun_fd);
        return 1;
    }

    pthread_create(&tx_tid, NULL, tx_thread_func, &tun_fd);
    pthread_create(&rx_tid, NULL, rx_thread_func, &tun_fd);

    printf("Running. Press Ctrl+C to stop.\n");
    while (g_running)
        sleep(1);

    g_running = 0;
    pthread_join(tx_tid, NULL);
    pthread_join(rx_tid, NULL);

    audio_cleanup(g_audio_handle);
    g_audio_handle = NULL;
    tun_close(tun_fd);
    printf("Exited.\n");
    return 0;
}
