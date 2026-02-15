/**
 * utils.c - 工具函数实现（CRC-16、调试打印）
 *
 * CRC 采用 CCITT 多项式 0x1021，初始值 0xFFFF，与常见通信协议一致。
 */

#include "utils.h"
#include <stdio.h>
#include <string.h>

/* CRC-16-CCITT 多项式: x^16 + x^12 + x^5 + 1 => 0x1021 */
#define CRC16_POLY  0x1021
#define CRC16_INIT  0xFFFF

/**
 * 计算 CRC-16 (CCITT) （Cyclic Redundancy Check）循环冗余校验
 * 逐字节处理，每字节 8 位从高到低与当前 CRC 按多项式做除法（异或移位）
 */
uint16_t crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = CRC16_INIT;
    size_t i;
    int b;

    if (!data)
        return crc;

    for (i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (b = 0; b < 8; b++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ CRC16_POLY;
            else
                crc = crc << 1;
        }
    }
    return crc;
}

/**
 * 以十六进制打印一段数据，便于调试帧内容
 * 格式: TAG: xx xx xx xx ...
 */
void debug_hex_dump(const char *tag, const uint8_t *data, size_t len)
{
    size_t i;
    if (!tag) tag = "";
    if (!data) return;
    printf("%s: ", tag);
    for (i = 0; i < len && i < 64; i++)  /* 最多打印 64 字节 */
        printf("%02x ", data[i]);
    if (len > 64)
        printf("... (%zu bytes total)", len);
    printf("\n");
}
