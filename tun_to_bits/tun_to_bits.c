/**
 * tun_to_bits.c - 主程序：同一可执行文件，不同参数独立跑每一步
 *
 * 用法：
 *   tun_to_bits --create [tun_name]  仅创建 TUN，验证成功后退出
 *   tun_to_bits --read [tun_name]    仅从 TUN 读一包，写入 output/ip.bin
 *   tun_to_bits --encapsulate        从 output/ip.bin 封装成帧，写入 output/frame.bin
 *   tun_to_bits --to-bits            从 output/frame.bin 转比特，写入 output/bits.bin
 *   tun_to_bits --test               测试模式：内置/testdata IP → 帧 → 比特（不依赖 TUN）
 *   tun_to_bits [tun_name]           全流程：创建 TUN → 读包 → 封装 → 转比特（需 root）
 */

#include "../include/common.h"
#include "../include/tun_dev.h"
#include "tun_create.h"
#include "packet_read.h"
#include "encapsulate.h"
#include "frame_to_bits.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define IP_OUT_PATH    "output/ip.bin"
#define FRAME_OUT_PATH "output/frame.bin"
#define BITS_OUT_PATH  "output/bits.bin"

static void print_hex(const char *prefix, const uint8_t *buf, int len)
{
    int i;
    printf("%s (%d bytes)\n", prefix, len);
    for (i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 16 == 0 || i == len - 1)
            printf("\n");
    }
}

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s --create [tun_name]   Create TUN only, verify and exit\n", prog);
    fprintf(stderr, "  %s --read [tun_name]     Read one packet from TUN -> output/ip.bin\n", prog);
    fprintf(stderr, "  %s --encapsulate         output/ip.bin -> output/frame.bin\n", prog);
    fprintf(stderr, "  %s --to-bits             output/frame.bin -> output/bits.bin\n", prog);
    fprintf(stderr, "  %s --test                Test: built-in IP -> frame -> bits (no TUN)\n", prog);
    fprintf(stderr, "  %s [tun_name]            Full pipeline (create -> read -> encapsulate -> to-bits)\n", prog);
}

/** 步骤 1：仅创建 TUN，验证成功 */
static int run_create(int argc, char *argv[])
{
    const char *tun_name = (argc >= 3) ? argv[2] : TUN_DEV_NAME;
    int fd = tun_create(tun_name);
    if (fd < 0) {
        fprintf(stderr, "TUN created failed. Try: sudo %s --create [tun_name]\n", argv[0]);
        return -1;
    }
    printf("TUN created successfully: %s (fd=%d)\n", tun_name, fd);
    tun_close(fd);
    return 0;
}

/** 步骤 2：仅从 TUN 读一包，写入 output/ip.bin */
static int run_read(int argc, char *argv[])
{
    uint8_t ip_buf[MAX_FRAME_PAYLOAD];
    const char *tun_name = (argc >= 3) ? argv[2] : TUN_DEV_NAME;
    int fd, n;
    FILE *fp;

    fd = tun_create(tun_name);
    if (fd < 0) {
        fprintf(stderr, "TUN opened failed. Try: sudo %s --read [tun_name]\n", argv[0]);
        return -1;
    }

    printf("Waiting for one IP packet on %s...\n", tun_name);
    n = packet_read(fd, ip_buf, MAX_FRAME_PAYLOAD);
    tun_close(fd);
    if (n <= 0) {
        fprintf(stderr, "Read packet failed or no data\n");
        return -1;
    }

    fp = fopen(IP_OUT_PATH, "wb");
    if (!fp) {
        fprintf(stderr, "Cannot write %s\n", IP_OUT_PATH);
        return -1;
    }
    fwrite(ip_buf, 1, (size_t)n, fp);
    fclose(fp);

    printf("Read packet successfully: %d bytes -> %s\n", n, IP_OUT_PATH);
    return 0;
}

/** 步骤 3：从 output/ip.bin 封装成帧，写入 output/frame.bin */
static int run_encapsulate(void)
{
    uint8_t ip_buf[MAX_FRAME_PAYLOAD];
    uint8_t frame_buf[MAX_FRAME_LEN];
    FILE *fp;
    int ip_len, frame_len;

    fp = fopen(IP_OUT_PATH, "rb");
    if (!fp) {
        fprintf(stderr, "Cannot read %s (run --read first)\n", IP_OUT_PATH);
        return -1;
    }
    ip_len = (int)fread(ip_buf, 1, MAX_FRAME_PAYLOAD, fp);
    fclose(fp);
    if (ip_len <= 0) {
        fprintf(stderr, "%s empty or read error\n", IP_OUT_PATH);
        return -1;
    }

    frame_len = encapsulate(ip_buf, ip_len, frame_buf);
    if (frame_len <= 0) {
        fprintf(stderr, "encapsulate failed\n");
        return -1;
    }

    fp = fopen(FRAME_OUT_PATH, "wb");
    if (!fp) {
        fprintf(stderr, "Cannot write %s\n", FRAME_OUT_PATH);
        return -1;
    }
    fwrite(frame_buf, 1, (size_t)frame_len, fp);
    fclose(fp);

    printf("Encapsulate successfully: IP %d bytes -> Frame %d bytes -> %s\n", ip_len, frame_len, FRAME_OUT_PATH);
    return 0;
}

/** 步骤 4：从 output/frame.bin 转比特，写入 output/bits.bin */
static int run_to_bits(void)
{
    uint8_t frame_buf[MAX_FRAME_LEN];
    uint8_t bits_buf[MAX_FRAME_LEN + 1];
    FILE *fp;
    int frame_len, nbits;

    fp = fopen(FRAME_OUT_PATH, "rb");
    if (!fp) {
        fprintf(stderr, "Cannot read %s (run --encapsulate first)\n", FRAME_OUT_PATH);
        return -1;
    }
    frame_len = (int)fread(frame_buf, 1, MAX_FRAME_LEN, fp);
    fclose(fp);
    if (frame_len <= 0) {
        fprintf(stderr, "%s empty or read error\n", FRAME_OUT_PATH);
        return -1;
    }

    frame_to_bits(frame_buf, frame_len, bits_buf, &nbits);

    fp = fopen(BITS_OUT_PATH, "wb");
    if (!fp) {
        fprintf(stderr, "Cannot write %s\n", BITS_OUT_PATH);
        return -1;
    }
    fwrite(bits_buf, 1, (size_t)((nbits + 7) / 8), fp);
    fclose(fp);

    printf("Convert frame to bits successfully: Frame %d bytes -> %d bits -> %s\n", frame_len, nbits, BITS_OUT_PATH);
    return 0;
}

/** 测试模式：内置/testdata IP -> 封装 -> 转比特，打印并写 output/ */
static int run_test(void)
{
    uint8_t ip_buf[MAX_FRAME_PAYLOAD];
    uint8_t frame_buf[MAX_FRAME_LEN];
    uint8_t bits_buf[MAX_FRAME_LEN + 1];
    int ip_len, frame_len, nbits;
    FILE *fp;

    fp = fopen("testdata/ip_packet.bin", "rb");
    if (fp) {
        ip_len = (int)fread(ip_buf, 1, MAX_FRAME_PAYLOAD, fp);
        fclose(fp);
        if (ip_len <= 0) {
            fprintf(stderr, "testdata/ip_packet.bin empty or read error\n");
            return -1;
        }
        printf("[Test] Read %d bytes from testdata/ip_packet.bin\n", ip_len);
    } else {
        static const uint8_t default_ip[] = {
            0x45, 0x00, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00,
            0x40, 0x01, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x01,
            0x0a, 0x00, 0x00, 0x02
        };
        ip_len = (int)(sizeof(default_ip) / sizeof(default_ip[0]));
        memcpy(ip_buf, default_ip, (size_t)ip_len);
        printf("[Test] Using built-in IP payload (%d bytes)\n", ip_len);
    }

    frame_len = encapsulate(ip_buf, ip_len, frame_buf);
    if (frame_len <= 0) {
        fprintf(stderr, "encapsulate failed\n");
        return -1;
    }

    frame_to_bits(frame_buf, frame_len, bits_buf, &nbits);

    printf("\n========== Frame result (Frame) ==========\n");
    print_hex("Frame (hex)", frame_buf, frame_len);

    printf("\n========== Bits result (Bits, first 64 bytes hex) ==========\n");
    {
        int show = (nbits + 7) / 8;
        if (show > 64) show = 64;
        print_hex("Bits (hex)", bits_buf, show);
        if ((nbits + 7) / 8 > 64)
            printf("... (total %d bytes, %d bits)\n", (nbits + 7) / 8, nbits);
    }

    fp = fopen(IP_OUT_PATH, "wb");
    if (fp) {
        fwrite(ip_buf, 1, (size_t)ip_len, fp);
        fclose(fp);
        printf("Wrote IP to %s\n", IP_OUT_PATH);
    }
    fp = fopen(FRAME_OUT_PATH, "wb");
    if (fp) {
        fwrite(frame_buf, 1, (size_t)frame_len, fp);
        fclose(fp);
        printf("Wrote frame to %s\n", FRAME_OUT_PATH);
    }
    fp = fopen(BITS_OUT_PATH, "wb");
    if (fp) {
        fwrite(bits_buf, 1, (size_t)((nbits + 7) / 8), fp);
        fclose(fp);
        printf("Wrote bits to %s\n", BITS_OUT_PATH);
    }

    printf("\nTest OK: IP %d bytes -> Frame %d bytes -> %d bits\n", ip_len, frame_len, nbits);
    return 0;
}

/** 全流程：创建 TUN -> 读包 -> 封装 -> 转比特 */
static int run_full(int argc, char *argv[])
{
    uint8_t ip_buf[MAX_FRAME_PAYLOAD];
    uint8_t frame_buf[MAX_FRAME_LEN];
    uint8_t bits_buf[MAX_FRAME_LEN + 1];
    int tun_fd, ip_len, frame_len, nbits;
    const char *tun_name = (argc >= 2) ? argv[1] : TUN_DEV_NAME;

    printf("Opening TUN %s, waiting for one IP packet...\n", tun_name);
    tun_fd = tun_create(tun_name);
    if (tun_fd < 0) {
        fprintf(stderr, "tun_create failed. Try: sudo %s [tun_name]\n", argv[0]);
        return -1;
    }

    ip_len = packet_read(tun_fd, ip_buf, MAX_FRAME_PAYLOAD);
    tun_close(tun_fd);
    if (ip_len <= 0) {
        fprintf(stderr, "packet_read failed or no data\n");
        return 1;
    }
    printf("Read %d bytes from TUN\n", ip_len);

    frame_len = encapsulate(ip_buf, ip_len, frame_buf);
    if (frame_len <= 0) {
        fprintf(stderr, "encapsulate failed\n");
        return 1;
    }

    frame_to_bits(frame_buf, frame_len, bits_buf, &nbits);

    printf("\n========== 帧结果 (Frame) ==========\n");
    print_hex("Frame (hex)", frame_buf, frame_len);

    printf("\n========== 比特流 (Bits, 前 64 字节 hex) ==========\n");
    {
        int show = (nbits + 7) / 8;
        if (show > 64) show = 64;
        print_hex("Bits (hex)", bits_buf, show);
        if ((nbits + 7) / 8 > 64)
            printf("... (total %d bytes)\n", (nbits + 7) / 8);
    }

    printf("\nOK: %d bytes IP -> Frame %d bytes -> %d bits\n", ip_len, frame_len, nbits);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--create") == 0) {
        return run_create(argc, argv) == 0 ? 0 : 1;
    }
    if (strcmp(argv[1], "--read") == 0) {
        return run_read(argc, argv) == 0 ? 0 : 1;
    }
    if (strcmp(argv[1], "--encapsulate") == 0) {
        return run_encapsulate() == 0 ? 0 : 1;
    }
    if (strcmp(argv[1], "--to-bits") == 0) {
        return run_to_bits() == 0 ? 0 : 1;
    }
    if (strcmp(argv[1], "--test") == 0 || strcmp(argv[1], "-t") == 0) {
        return run_test() == 0 ? 0 : 1;
    }

    /* 无 -- 前缀则当作全流程，第一个参数为 tun_name */
    return run_full(argc, argv) == 0 ? 0 : 1;
}
