/**
 * common.h - IP over Sound 项目全局常量与配置
 *
 * 本头文件集中定义全项目共用的参数，便于调试和修改。
 * 修改采样率、缓冲区大小、FSK 频率等时只需改此文件。
 */

#ifndef COMMON_H
#define COMMON_H

/* ========== 音频参数 ========== */
/** 采样率 (Hz)，必须与声卡支持的一致，FSK 载波频率须 < SAMPLE_RATE/2 */
#define SAMPLE_RATE         44100   /** 表示每秒采样44100次 */

/** 每次从声卡读/写的采样帧大小，影响延迟与 CPU 占用 */
#define AUDIO_FRAMES_PER_BUFFER  1024   /** 表示每次从声卡读/写的采样帧大小为1024次 */

/** 音频采样数据类型：每个采样为 [-1.0, 1.0] 的浮点数 */
typedef float sample_t;

/* ========== FSK 调制参数 (物理层) ========== */
/** FSK是频移键控，是一种数字调制技术，通过改变载波频率来传输数字信号。FSK有2种频率，0和1。
 * 比特0对应1200Hz，比特1对应2400Hz。
/** 比特 0 对应的载波频率 (Hz) */
#define FSK_FREQ_0         1200

/** 比特 1 对应的载波频率 (Hz) */
#define FSK_FREQ_1         2400

/** 每秒发送的比特数 (bps)，影响每比特对应的采样点数 */
#define FSK_BAUD_RATE      1200  /** 表示每秒发送1200个比特 ，每个比特的持续时间为1/1200秒*/

/** 每比特对应的采样点数 = SAMPLE_RATE / FSK_BAUD_RATE */
#define SAMPLES_PER_BIT    (SAMPLE_RATE / FSK_BAUD_RATE)

/* ========== 协议/帧参数 (链路层) ========== */
/**这边帧指的是封装后的数据帧，包括同步字、长度、载荷、CRC。 在声波链路上传输时，把“一块要传的数据”包成的一个带格式的单元。*/
/** 最大一帧的载荷长度（即单个 IP 包最大字节数），与 TUN MTU 一致 */
/** “帧” = 同步字 + 长度 + IP 包（载荷）+ CRC 这一整块，是声波链路上一次传输的基本单位。*/
/** 和“包”的关系（帮助理解）
IP 包：网络层概念，内核/TUN 给你的就是“一个 IP 包”。
帧：链路层概念，是“为了在声波这条链路上传一个 IP 包，而把它装进去的那个壳”。
可以记成：
发的时候：一个 IP 包 → 装进一帧（加头加尾）→ 变成比特流 → 调制 → 声波。
收的时候：声波 → 解调 → 比特流 → 找到帧头、按长度取出一帧 → 校验 CRC → 从帧里取出 IP 包交给 TUN。*/
#define MAX_FRAME_PAYLOAD  1500  /** 表示一帧的最大载荷长度为1500字节*/

/** 帧头同步字长度（字节），用于接收端定位帧起始 */
#define SYNC_LEN           2  /** 表示同步字长度为2字节*/

/** 同步字节取值（0x7E 为 HDLC 常用，避免与常见数据冲突） */
#define SYNC_BYTE          0x7E  /** 表示同步字节取值为0x7E*/

/** 长度字段占用字节数 */
#define LEN_FIELD_BYTES    2  /** 表示长度字段占用2字节，足够表示1500字节的载荷长度*/

/** CRC 校验占用字节数 (CRC-16) */
#define CRC_BYTES          2

/** 帧头总长度：同步字 + 长度 = SYNC_LEN + LEN_FIELD_BYTES */
#define FRAME_HEADER_LEN   (SYNC_LEN + LEN_FIELD_BYTES)

/** 一帧最大字节数（头 + 载荷 + CRC） */
#define MAX_FRAME_LEN      (FRAME_HEADER_LEN + MAX_FRAME_PAYLOAD + CRC_BYTES)

/* ========== TUN 设备 ========== */
/** 默认 TUN 设备名称，若 /dev/net/tun 已存在则使用 tun0 等 */
#define TUN_DEV_NAME        "tun0"

#endif /* COMMON_H */
