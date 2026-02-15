/**
 * protocol.c - 帧封装与解析实现
 *
 * 帧格式: [SYNC_BYTE x SYNC_LEN][长度 2 字节 大端][载荷][CRC16 2 字节 大端]
 * 长度字段 = 载荷字节数（不含头与 CRC），便于接收端分配缓冲区并校验 CRC。
 */

#include "protocol.h"
#include "utils.h"
#include <string.h>

/**
 * 封装：同步字 + 长度(大端) + 载荷 + CRC16(大端)
 */
 /**
  * 封装：同步字 + 长度(大端) + 载荷 + CRC16(大端)
  * @param payload 载荷，即要封装的IP包数据
  * @param payload_len 载荷长度
  * @param frame_out 输出帧缓冲区
  * @return 输出帧的长度
  */
int protocol_encapsulate(const uint8_t *payload, int payload_len, uint8_t *frame_out)
{
    uint16_t crc;
    int i;

    if (!payload || !frame_out || payload_len <= 0 || payload_len > MAX_FRAME_PAYLOAD)
        return 0;

    /* 1. 同步字 */
    for (i = 0; i < SYNC_LEN; i++)
        frame_out[i] = SYNC_BYTE; /** 这个是*(frame_out + i)的简写,改的是内容 */

    /* 2. 长度（大端） */
    frame_out[SYNC_LEN]     = (payload_len >> 8) & 0xFF; /** 取高位+位与掩码0xFF */
    frame_out[SYNC_LEN + 1] = payload_len & 0xFF; /** 取低位+位与掩码0xFF */

    /* 3. 拷贝载荷 */
    memcpy(frame_out + FRAME_HEADER_LEN, payload, payload_len); /** 拷贝载荷到帧缓冲区 ，从第4个字节开始，长度为payload_len*/

    /* 4. CRC：对「长度+载荷」计算，放在帧尾（大端） */
    crc = crc16(frame_out + SYNC_LEN, LEN_FIELD_BYTES + payload_len);
    frame_out[FRAME_HEADER_LEN + payload_len]     = (crc >> 8) & 0xFF;
    frame_out[FRAME_HEADER_LEN + payload_len + 1] = crc & 0xFF;

    return FRAME_HEADER_LEN + payload_len + CRC_BYTES;
}

/**
 * 解封装：校验 CRC 后取出载荷
 * @param frame 输入帧缓冲区
 * @param frame_len 帧长度
 * @param payload_out 输出载荷（IP 包）缓冲区
 * @param max_payload 缓冲区最大长度
 * @return 输出载荷的长度
 */
int protocol_decapsulate(const uint8_t *frame, int frame_len,
                         uint8_t *payload_out, int max_payload)
{
    uint16_t len_u16, crc_stored, crc_computed;

    if (!frame || !payload_out || frame_len < FRAME_HEADER_LEN + CRC_BYTES)
        return -1;

    /* 长度（大端） */
    len_u16 = (frame[SYNC_LEN] << 8) | frame[SYNC_LEN + 1];
    if (len_u16 <= 0 || len_u16 > MAX_FRAME_PAYLOAD)
        return -1;
    if (frame_len < FRAME_HEADER_LEN + len_u16 + CRC_BYTES)
        return -1;
    if (len_u16 > max_payload)
        return -1;

    /* 校验 CRC：对「长度+载荷」计算，与帧尾两字节比较 */
    crc_computed = crc16(frame + SYNC_LEN, LEN_FIELD_BYTES + len_u16);
    crc_stored   = (frame[FRAME_HEADER_LEN + len_u16] << 8)
                   | frame[FRAME_HEADER_LEN + len_u16 + 1];
    if (crc_computed != crc_stored)
        return -1;  /* CRC 错误，丢弃 */

    memcpy(payload_out, frame + FRAME_HEADER_LEN, len_u16); /** 拷贝载荷到输出缓冲区 ，从第4个字节开始，长度为len_u16*/
    return (int)len_u16;
}

/**
 * 在比特流中查找同步：连续 SYNC_LEN 字节的同步字（按比特匹配）
 * bits 中每字节 8 比特，高位在前。
 */
int protocol_find_sync(const uint8_t *bits, int nbits)
{
    int i, j, k;
    int need_bits = SYNC_LEN * 8;

    if (!bits || nbits < need_bits)
        return -1;

    for (i = 0; i <= nbits - need_bits; i++) {
        for (j = 0; j < SYNC_LEN; j++) {
            int byte_val = 0;
            for (k = 0; k < 8; k++) {
                int bit_idx = i + j * 8 + k;
                int byte_idx = bit_idx / 8;
                int bit_in_byte = 7 - (bit_idx % 8);
                if (bits[byte_idx] & (1 << bit_in_byte))
                    byte_val |= (1 << (7 - k));
            }
            if (byte_val != SYNC_BYTE)
                break;
        }
        if (j == SYNC_LEN)
            return i;
    }
    return -1;
}
