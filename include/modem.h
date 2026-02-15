/**
 * modem.h - FSK 调制解调器接口（物理层）
 *
 * 调制：将比特流转换为音频波形（每个比特对应一段正弦波）
 * 解调：将音频波形转换回比特流
 * 不关心帧结构，只做 0/1 与波形的转换。
 */

 /**已有比特”在哪儿？
网络层：只有 IP 包（字节），从 TUN 读出来，或要写回 TUN。
链路层：protocol 把 IP 包封装成帧（加同步、长度、CRC），此时还是字节；然后为了上物理层，把这一帧按位展开成 0/1 比特流。
“已有比特” = 就是这串 链路层帧的比特表示：
“来自 protocol 封装好的帧” → 把帧的每个字节拆成 8 个比特 → 得到的一长串 0/1。
所以：
网络层 = IP 包（帧里的“载荷”部分）。
已有比特 = 整个帧（含同步、长度、IP 包、CRC）的比特形式，属于链路层，是准备送给物理层（modem 调制）的输入。 */

#ifndef MODEM_H
#define MODEM_H

#include "common.h"
#include <stdint.h>

/** 调制器状态/句柄，内部保存相位等，对外不透明 */
typedef void* modem_tx_handle_t;

/** 解调器状态/句柄，内部保存缓冲区与状态 */
typedef void* modem_rx_handle_t;

/**
 * 创建调制器（用于发送），初始化调制器内部状态
 * @return 句柄，失败返回 NULL
 */
modem_tx_handle_t modem_tx_create(void);

/**
 * 将一段比特流调制为音频采样，写入 out_buf
 * @param h       modem_tx_create 返回的句柄
 * @param bits    比特数组，每个字节存 8 个比特，高位先发
 * @param nbits   比特总数
 * @param out_buf 输出采样缓冲区，需预分配足够空间 (约 nbits * SAMPLES_PER_BIT)，缓冲区的大小用采样数来表示
 * @return        实际写入的采样数
 */
int modem_tx_modulate(modem_tx_handle_t h, const uint8_t *bits, int nbits, sample_t *out_buf);

/**
 * 销毁调制器
 * @param h 句柄
 */
void modem_tx_destroy(modem_tx_handle_t h);

/**
 * 创建解调器（用于接收）
 * @return 句柄，失败返回 NULL
 */
modem_rx_handle_t modem_rx_create(void);

/**
 * 将一段音频采样解调为比特，写入 bits 缓冲区
 * @param h        modem_rx_create 返回的句柄
 * @param samples  输入采样
 * @param nsamples 采样数
 * @param bits     输出比特数组（每字节 8 比特）
 * @param max_bits 最多输出的比特数（避免越界）
 * @return         实际输出的比特数
 */
int modem_rx_demodulate(modem_rx_handle_t h, const sample_t *samples, int nsamples,
                        uint8_t *bits, int max_bits);

/**
 * 销毁解调器
 * @param h 句柄
 */
void modem_rx_destroy(modem_rx_handle_t h);

#endif /* MODEM_H */
