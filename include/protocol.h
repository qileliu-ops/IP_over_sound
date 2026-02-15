/**
 * protocol.h - 帧封装与解析（链路层）
 *
 * 发送：给 IP 包加帧头（同步字 + 长度）+ 帧尾（CRC），得到一帧字节流
 * 接收：从比特流中找同步字、取长度、校验 CRC，拆出 IP 包
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "common.h"
#include <stdint.h>
#include <stddef.h>  /* size_t */

/**
 * 将 IP 包封装为一帧（同步 + 长度 + 载荷 + CRC），输出为字节数组
 * @param payload    IP 包数据
 * @param payload_len 包长度
 * @param frame_out  输出缓冲区，至少 FRAME_HEADER_LEN + payload_len + CRC_BYTES
 * @return           输出帧的总字节数，失败返回 0
 */
int protocol_encapsulate(const uint8_t *payload, int payload_len, uint8_t *frame_out);

/**
 * 从字节流中解析一帧：找同步字、读长度、校验 CRC，拆出载荷
 * @param frame     一帧完整数据（含头+载荷+CRC）
 * @param frame_len 帧长度
 * @param payload_out 输出载荷（IP 包）缓冲区
 * @param max_payload  缓冲区最大长度
 * @return          成功返回载荷字节数，失败返回 -1（如 CRC 错误）
 */
int protocol_decapsulate(const uint8_t *frame, int frame_len,
                         uint8_t *payload_out, int max_payload);

/**
 * 在比特流中查找帧同步位置（连续 SYNC_LEN 个同步字节的起始比特下标）
 * @param bits    比特数组（每字节 8 比特，高位在前）
 * @param nbits   总比特数
 * @return        同步起始比特下标，未找到返回 -1
 */
int protocol_find_sync(const uint8_t *bits, int nbits);

#endif /* PROTOCOL_H */
