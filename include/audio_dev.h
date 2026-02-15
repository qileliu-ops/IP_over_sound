/**
 * audio_dev.h - 声卡设备接口（基于 PortAudio）
 *
 * 负责初始化声卡、写入采样到扬声器、从麦克风读取采样。
 * 采样率为 common.h 中的 SAMPLE_RATE，数据类型为 sample_t (float)。
 */

#ifndef AUDIO_DEV_H
#define AUDIO_DEV_H

#include "common.h"

/** 音频设备句柄，内部为 PortAudio 流指针，对外不透明 */
typedef void* audio_handle_t;

/**
 * 初始化音频：打开默认输入/输出设备，按 SAMPLE_RATE 和 AUDIO_FRAMES_PER_BUFFER 配置
 * @return 成功返回句柄，失败返回 NULL
 */
audio_handle_t audio_init(void);  
/** 初始化音频设备，打开默认输入/输出设备，按 SAMPLE_RATE 和 AUDIO_FRAMES_PER_BUFFER 配置
/**这个audio_init函数定义在audio_dev.c文件中，函数返回一个audio_handle_t类型的句柄，用于后续的音频操作。*/

/**
 * 向扬声器写入一帧采样（播放）
 * @param h      audio_init 返回的句柄
 * @param buf    采样数据，长度为 nframes
 * @param nframes 本帧采样数
 * @return       0 成功，非 0 失败
 */
int audio_write(audio_handle_t h, const sample_t *buf, int nframes);
/**这个const sample_t *buf表示输入的采样数据，int nframes表示本帧采样数，const是常量指针，表示buf指向的内存地址不能被修改。*/

/**
 * 从麦克风读取一帧采样（录音）
 * @param h      audio_init 返回的句柄
 * @param buf    输出缓冲区，至少 nframes 个 sample_t
 * @param nframes 要读取的采样数
 * @return       实际读取的采样数，失败返回 <0
 */
int audio_read(audio_handle_t h, sample_t *buf, int nframes);

/**
 * 关闭音频设备并释放资源
 * @param h audio_init 返回的句柄
 */
void audio_cleanup(audio_handle_t h);

#endif /* AUDIO_DEV_H */
