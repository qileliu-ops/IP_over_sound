/**
 * modem.c - FSK 调制解调实现
 *
 * 调制：每个比特对应 SAMPLES_PER_BIT 个采样，0 用 FSK_FREQ_0 Hz 正弦，1 用 FSK_FREQ_1 Hz 正弦
 * 解调：对每比特时长内的采样做简单鉴频（能量或过零），判 0/1
 */

#include "modem.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ========== 调制器：内部保存相位，保证波形连续 ========== */
struct modem_tx {
    double phase0;  /* 当前 0 载波相位 (弧度) */
    double phase1;  /* 当前 1 载波相位 (弧度) */
};

modem_tx_handle_t modem_tx_create(void)
{
    struct modem_tx *tx = (struct modem_tx *)calloc(1, sizeof(struct modem_tx));
    return (modem_tx_handle_t)tx;
}

/* 生成一段正弦波，振幅 0.3 避免削顶，并更新相位 */
/** 这边sample_t *out是输出采样缓冲区，out[i]是第i个采样，out[i]的类型是sample_t，即浮点数。定义时不区分是一个采样还是多个采样的第一个，在调用前要预先分配足够空间。*/
static void gen_sine(double freq, int nsamples, sample_t *out,
                     double *phase_inout)
{
    double phase = *phase_inout;
    double step = 2.0 * M_PI * freq / SAMPLE_RATE;
    int i;

    for (i = 0; i < nsamples; i++) {
        out[i] = (sample_t)(0.3 * sin(phase));
        phase += step;
    }
    /* 归一化到 [0, 2π) 避免累积误差 */
    while (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;
    while (phase < 0) phase += 2.0 * M_PI;
    *phase_inout = phase;
}

/**
 * 将 bits 缓冲区中的 nbits 个比特调制为波形
 * bits 中每字节 8 比特，高位先发
 */
int modem_tx_modulate(modem_tx_handle_t h, const uint8_t *bits, int nbits, sample_t *out_buf)
{
    struct modem_tx *tx = (struct modem_tx *)h;
    int i, bit_idx, byte_idx, bit_in_byte;
    int out_idx = 0;

    if (!tx || !bits || !out_buf || nbits <= 0)
        return 0;

    for (bit_idx = 0; bit_idx < nbits; bit_idx++) {
        byte_idx   = bit_idx / 8;
        bit_in_byte = 7 - (bit_idx % 8);
        if (bits[byte_idx] & (1 << bit_in_byte))
            gen_sine(FSK_FREQ_1, SAMPLES_PER_BIT, out_buf + out_idx, &tx->phase1);
        else
            gen_sine(FSK_FREQ_0, SAMPLES_PER_BIT, out_buf + out_idx, &tx->phase0);
        out_idx += SAMPLES_PER_BIT;
    }
    return out_idx;
}

void modem_tx_destroy(modem_tx_handle_t h)
{
    free(h);
}

/* ========== 解调器：每比特采样内简单能量/过零判 0/1 ，判断波形的抖动幅度========== */
struct modem_rx {
    /* 可扩展：保存未处理完的采样用于下一段拼接 */
    sample_t *remain_buf;
    int remain_len;
};

modem_rx_handle_t modem_rx_create(void)
{
    struct modem_rx *rx = (struct modem_rx *)calloc(1, sizeof(struct modem_rx));
    if (!rx) return NULL;
    /* 预分配一小段缓冲区，用于跨块拼接（此处简化，可不用） */
    rx->remain_buf = NULL;
    rx->remain_len = 0;
    return (modem_rx_handle_t)rx;
}

/**
 * 对 SAMPLES_PER_BIT 个采样判 0 或 1：比较 1200Hz 与 2400Hz 分量能量
 * 简化实现：用该段内“过零次数”近似区分低频(0)与高频(1)
 */
static int demodulate_bit(const sample_t *samples, int nsamples)
{
    int zeros = 0, i;
    double avg_abs = 0;
    /* 过零计数：低频过零少，高频过零多 */
    for (i = 1; i < nsamples; i++) {
        if ((samples[i-1] >= 0 && samples[i] < 0) || (samples[i-1] < 0 && samples[i] >= 0))
            zeros++;
        avg_abs += fabs(samples[i]);
    }
    avg_abs /= nsamples;
    /* 粗略阈值：过零多则判为 1 (高频) */
    return (zeros > nsamples / 4) ? 1 : 0;
}

int modem_rx_demodulate(modem_rx_handle_t h, const sample_t *samples, int nsamples,
                        uint8_t *bits, int max_bits)
{
    struct modem_rx *rx = (struct modem_rx *)h;
    int nbits = 0;
    int i, byte_idx, bit_in_byte;

    if (!rx || !samples || !bits || nsamples < SAMPLES_PER_BIT || max_bits <= 0)
        return 0;

    for (i = 0; i + SAMPLES_PER_BIT <= nsamples && nbits < max_bits; i += SAMPLES_PER_BIT) {
        int bit = demodulate_bit(samples + i, SAMPLES_PER_BIT);
        byte_idx   = nbits / 8;
        bit_in_byte = 7 - (nbits % 8);
        if (bit)
            bits[byte_idx] |= (1 << bit_in_byte);
        else
            bits[byte_idx] &= ~(1 << bit_in_byte);
        nbits++;
    }
    return nbits;
}

void modem_rx_destroy(modem_rx_handle_t h)
{
    struct modem_rx *rx = (struct modem_rx *)h;
    if (rx) {
        free(rx->remain_buf);
        free(rx);
    }
}
