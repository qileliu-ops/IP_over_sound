/**
 * audio_dev.c - 声卡设备实现（PortAudio）
 *
 * 使用 PortAudio 打开默认输入/输出设备，按 SAMPLE_RATE 和
 * AUDIO_FRAMES_PER_BUFFER 进行读写。编译需链接 -lportaudio。
 */

#include "audio_dev.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <portaudio.h>

/** 在PortAudio的API中，一帧是一个时间点所有通道的采样，和链接层的帧不是一回事。*/

/** 内部句柄：保存 PaStream 指针 */
struct audio_handle {
    PaStream *stream_in;   /* 麦克风输入流 */
    PaStream *stream_out;  /* 扬声器输出流 */
    int opened;            /* 是否已成功打开 */
};


/** 初始化音频设备，打开默认输入/输出设备，按 SAMPLE_RATE 和 AUDIO_FRAMES_PER_BUFFER 配置 */
audio_handle_t audio_init(void)
{
    struct audio_handle *h;
    PaError err;

    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "audio_init: Pa_Initialize failed: %s\n", Pa_GetErrorText(err));
        return NULL;
    }

    h = (struct audio_handle *)calloc(1, sizeof(struct audio_handle));
    if (!h) {
        Pa_Terminate();
        return NULL;
    }

    /* 打开默认输入设备（麦克风） */
    err = Pa_OpenDefaultStream(&h->stream_in,
                               1,   /* 单声道输入 */
                               0,   /* 无输出 */
                               paFloat32,
                               SAMPLE_RATE,
                               AUDIO_FRAMES_PER_BUFFER,
                               NULL, NULL);
    if (err != paNoError) {
        fprintf(stderr, "audio_init: OpenDefaultStream (input) failed: %s\n", Pa_GetErrorText(err));
        free(h);
        Pa_Terminate();
        return NULL;
    }

    /* 打开默认输出设备（扬声器） */
    err = Pa_OpenDefaultStream(&h->stream_out,
                               0,   /* 无输入 */
                               1,   /* 单声道输出 */
                               paFloat32,
                               SAMPLE_RATE,
                               AUDIO_FRAMES_PER_BUFFER,
                               NULL, NULL);
    if (err != paNoError) {
        fprintf(stderr, "audio_init: OpenDefaultStream (output) failed: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(h->stream_in);
        free(h);
        Pa_Terminate();
        return NULL;
    }

    err = Pa_StartStream(h->stream_in);
    if (err != paNoError) {
        fprintf(stderr, "audio_init: StartStream input failed: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(h->stream_in);
        Pa_CloseStream(h->stream_out);
        free(h);
        Pa_Terminate();
        return NULL;
    }

    err = Pa_StartStream(h->stream_out);
    if (err != paNoError) {
        fprintf(stderr, "audio_init: StartStream output failed: %s\n", Pa_GetErrorText(err));
        Pa_StopStream(h->stream_in);
        Pa_CloseStream(h->stream_in);
        Pa_CloseStream(h->stream_out);
        free(h);
        Pa_Terminate();
        return NULL;
    }

    h->opened = 1;
    return (audio_handle_t)h;
}

/** 写入nframes个采样到扬声器 */
int audio_write(audio_handle_t handle, const sample_t *buf, int nframes)
{
    struct audio_handle *h = (struct audio_handle *)handle;
    PaError err;

    if (!h || !h->opened || !buf || nframes <= 0)
        return -1;

    err = Pa_WriteStream(h->stream_out, buf, nframes);
    if (err != paNoError && err != paOutputUnderflowed) {
        fprintf(stderr, "audio_write: %s\n", Pa_GetErrorText(err));
        return -1;
    }
    return 0;
}

/** 读取nframes个采样从麦克风 */
int audio_read(audio_handle_t handle, sample_t *buf, int nframes)
{
    struct audio_handle *h = (struct audio_handle *)handle;
    PaError err;
    unsigned long n = (unsigned long)nframes;

    if (!h || !h->opened || !buf || nframes <= 0)
        return -1;

    err = Pa_ReadStream(h->stream_in, buf, n);
    if (err != paNoError && err != paInputOverflowed) {
        fprintf(stderr, "audio_read: %s\n", Pa_GetErrorText(err));
        return -1;
    }
    return (int)n;
}

/** 关闭音频设备并释放资源 */
void audio_cleanup(audio_handle_t handle)
{
    struct audio_handle *h = (struct audio_handle *)handle;

    if (!h) return;

    if (h->opened) {
        if (h->stream_in)  { Pa_StopStream(h->stream_in);  Pa_CloseStream(h->stream_in);  h->stream_in = NULL; }
        if (h->stream_out) { Pa_StopStream(h->stream_out); Pa_CloseStream(h->stream_out); h->stream_out = NULL; }
        h->opened = 0;
    }
    free(h);
    Pa_Terminate();
}
