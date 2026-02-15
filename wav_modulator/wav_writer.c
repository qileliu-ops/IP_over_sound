/**
 * wav_writer.c - 将 float 采样写入 WAV 文件（16-bit PCM）
 */

#include "wav_writer.h"
#include "../include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* WAV 格式：RIFF + fmt + data，16-bit PCM 单声道 */
static int write_wav_header(FILE *fp, int nsamples)
{
    uint8_t riff[4] = { 'R', 'I', 'F', 'F' };
    uint32_t file_size = 36 + (uint32_t)nsamples * 2; /* 36 + data bytes */
    uint8_t wave[4] = { 'W', 'A', 'V', 'E' };
    uint8_t fmt[4]  = { 'f', 'm', 't', ' ' };
    uint32_t fmt_len = 16;
    uint16_t audio_format = 1;   /* PCM */
    uint16_t num_channels = 1;
    uint32_t sample_rate = (uint32_t)SAMPLE_RATE;
    uint32_t byte_rate = sample_rate * 2;
    uint16_t block_align = 2;
    uint16_t bits_per_sample = 16;
    uint8_t data[4] = { 'd', 'a', 't', 'a' };
    uint32_t data_size = (uint32_t)nsamples * 2;

    if (fwrite(riff, 1, 4, fp) != 4) return -1;
    if (fwrite(&file_size, 1, 4, fp) != 4) return -1;
    if (fwrite(wave, 1, 4, fp) != 4) return -1;
    if (fwrite(fmt, 1, 4, fp) != 4) return -1;
    if (fwrite(&fmt_len, 1, 4, fp) != 4) return -1;
    if (fwrite(&audio_format, 1, 2, fp) != 2) return -1;
    if (fwrite(&num_channels, 1, 2, fp) != 2) return -1;
    if (fwrite(&sample_rate, 1, 4, fp) != 4) return -1;
    if (fwrite(&byte_rate, 1, 4, fp) != 4) return -1;
    if (fwrite(&block_align, 1, 2, fp) != 2) return -1;
    if (fwrite(&bits_per_sample, 1, 2, fp) != 2) return -1;
    if (fwrite(data, 1, 4, fp) != 4) return -1;
    if (fwrite(&data_size, 1, 4, fp) != 4) return -1;
    return 0;
}

int wav_write(const float *samples, int nsamples, const char *path)
{
    FILE *fp;
    int i;
    int16_t s16;

    if (!samples || nsamples <= 0 || !path)
        return -1;

    fp = fopen(path, "wb");
    if (!fp)
        return -1;

    if (write_wav_header(fp, nsamples) != 0) {
        fclose(fp);
        return -1;
    }

    for (i = 0; i < nsamples; i++) {
        float f = samples[i];
        if (f > 1.0f) f = 1.0f;
        if (f < -1.0f) f = -1.0f;
        s16 = (int16_t)(f * 32767.f);
        if (fwrite(&s16, 1, 2, fp) != 2) {
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return 0;
}
