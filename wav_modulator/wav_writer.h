/**
 * wav_writer.h - 将 float 采样写入 WAV 文件（16-bit PCM，单声道）
 * 与 common.h 的 SAMPLE_RATE 一致，供 bits_to_wav 使用。
 */

#ifndef WAV_WRITER_H
#define WAV_WRITER_H

/**
 * 将一段 float 采样写入 WAV 文件（单声道，16-bit PCM，44100 Hz）
 * @param samples  采样数组（float，约 [-1.0, 1.0]，本项目中调制输出约 [-0.3, 0.3]）
 * @param nsamples 采样个数
 * @param path     输出文件路径
 * @return         0 成功，-1 失败
 */
int wav_write(const float *samples, int nsamples, const char *path);

#endif /* WAV_WRITER_H */
