/**
 * bits_to_wav.c - 比特流 → FSK 调制 → WAV 文件
 *
 * 用法：
 *   bits_to_wav <input.bin> <output.wav>  从文件读比特流，调制后写 WAV
 *   bits_to_wav --test                    内置测试：生成短比特流，写入 output/test.wav
 *
 * 比特流文件格式：原始字节，每字节 8 比特，高位先发（与 modem 约定一致）。
 */

#include "../include/common.h"
#include "../include/modem.h"
#include "wav_writer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int read_bits_from_file(const char *path, uint8_t *bits_out, int max_bytes, int *nbits_out)
{
    FILE *fp;
    int n;

    fp = fopen(path, "rb");
    if (!fp) return -1;
    n = (int)fread(bits_out, 1, max_bytes, fp);
    fclose(fp);
    if (n <= 0) return -1;
    *nbits_out = n * 8;
    return 0;
}

/** 内置测试：写入较长比特流，调制后写 output/test.wav */
static int run_test(void)
{
#define TEST_BITS_BUF_SIZE  1024
    uint8_t bits_buf[TEST_BITS_BUF_SIZE];
    int nbits;
    int i;
    size_t max_samples;
    sample_t *samples_buf;
    modem_tx_handle_t mod_tx;
    const char *out_path = "output/test.wav";

    /* 原短测试比特流（已注释）：
     * bits_buf[0] = 0x7E;
     * bits_buf[1] = 0x7E;
     * bits_buf[2] = 0x00;
     * nbits = 24;
     */

    /* 新的长测试比特流：同步头 + 重复模式，共 TEST_BITS_BUF_SIZE 字节 */
    bits_buf[0] = 0x7E;
    bits_buf[1] = 0x7E;
    for (i = 2; i < TEST_BITS_BUF_SIZE; i++) {
        /* 交替 0x55/0xAA 使高低频交替，便于听出节奏；中间穿插 0x00/0xFF */
        if (i % 4 == 2) bits_buf[i] = 0x55;
        else if (i % 4 == 3) bits_buf[i] = 0xAA;
        else if (i % 4 == 0) bits_buf[i] = 0x00;
        else bits_buf[i] = 0xFF;
    }
    nbits = TEST_BITS_BUF_SIZE * 8;

    mod_tx = modem_tx_create();
    if (!mod_tx) {
        fprintf(stderr, "modem_tx_create failed\n");
        return -1;
    }

    max_samples = (size_t)nbits * SAMPLES_PER_BIT;
    samples_buf = (sample_t *)malloc(max_samples * sizeof(sample_t));
    if (!samples_buf) {
        modem_tx_destroy(mod_tx);
        fprintf(stderr, "malloc samples_buf failed\n");
        return -1;
    }

    int nsamples = modem_tx_modulate(mod_tx, bits_buf, nbits, samples_buf);
    modem_tx_destroy(mod_tx);
    if (nsamples <= 0) {
        free(samples_buf);
        fprintf(stderr, "modem_tx_modulate failed\n");
        return -1;
    }

    if (wav_write(samples_buf, nsamples, out_path) != 0) {
        free(samples_buf);
        fprintf(stderr, "wav_write failed: %s\n", out_path);
        return -1;
    }

    free(samples_buf);
    printf("Test OK: %d bits -> %d samples -> %s\n", nbits, nsamples, out_path);
    return 0;
}

int main(int argc, char *argv[])
{
    uint8_t *bits_buf;
    sample_t *samples_buf;
    modem_tx_handle_t mod_tx;
    int nbits = 0, nsamples, max_bytes;
    size_t max_samples;

    if (argc >= 2 && (strcmp(argv[1], "--test") == 0 || strcmp(argv[1], "-t") == 0)) {
        return run_test() == 0 ? 0 : 1;
    }

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.bin> <output.wav>\n", argv[0]);
        fprintf(stderr, "   or: %s --test\n", argv[0]);
        return 1;
    }

    max_bytes = 1024 * 1024; /* 最多 1MB 比特流文件 */
    bits_buf = (uint8_t *)malloc(max_bytes);
    if (!bits_buf) {
        fprintf(stderr, "malloc bits_buf failed\n");
        return 1;
    }

    if (read_bits_from_file(argv[1], bits_buf, max_bytes, &nbits) != 0) {
        fprintf(stderr, "Failed to read: %s\n", argv[1]);
        free(bits_buf);
        return 1;
    }

    mod_tx = modem_tx_create();
    if (!mod_tx) {
        free(bits_buf);
        fprintf(stderr, "modem_tx_create failed\n");
        return 1;
    }

    max_samples = (size_t)nbits * SAMPLES_PER_BIT;
    samples_buf = (sample_t *)malloc(max_samples * sizeof(sample_t));
    if (!samples_buf) {
        modem_tx_destroy(mod_tx);
        free(bits_buf);
        fprintf(stderr, "malloc samples_buf failed\n");
        return 1;
    }

    nsamples = modem_tx_modulate(mod_tx, bits_buf, nbits, samples_buf);
    modem_tx_destroy(mod_tx);
    free(bits_buf);
    if (nsamples <= 0) {
        free(samples_buf);
        fprintf(stderr, "modem_tx_modulate failed\n");
        return 1;
    }

    if (wav_write(samples_buf, nsamples, argv[2]) != 0) {
        free(samples_buf);
        fprintf(stderr, "wav_write failed: %s\n", argv[2]);
        return 1;
    }

    free(samples_buf);
    printf("OK: %d bits -> %d samples -> %s\n", nbits, nsamples, argv[2]);
    return 0;
}
