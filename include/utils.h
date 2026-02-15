/**
 * utils.h - 工具函数接口（CRC 校验、调试打印等）
 *
 * 供 protocol、main 等模块调用，不依赖具体硬件。
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

/**
 * 计算 CRC-16 (CCITT)，用于帧尾校验
 * @param data 待校验数据指针
 * @param len  数据长度（字节）
 * @return     16 位 CRC 值
 */
uint16_t crc16(const uint8_t *data, size_t len);

/**
 * 调试打印：以十六进制打印一段数据（可选，用于排查帧内容）
 * @param tag  前缀字符串，如 "TX" / "RX"
 * @param data 数据指针
 * @param len  长度
 */
void debug_hex_dump(const char *tag, const uint8_t *data, size_t len);

#endif /* UTILS_H */
