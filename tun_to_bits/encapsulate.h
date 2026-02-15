/**
 * encapsulate.h - 将 IP 包封装成帧（同步字 + 长度 + 载荷 + CRC）
 */

#ifndef ENCAPSULATE_H
#define ENCAPSULATE_H

#include <stdint.h>

/**
 * 把 IP 包封装为一帧
 * @param ip_payload  IP 包数据
 * @param ip_len      包长度
 * @param frame_out   输出帧缓冲区（需足够大，见 common.h MAX_FRAME_LEN）
 * @return            输出帧的字节数，失败返回 0
 */
int encapsulate(const uint8_t *ip_payload, int ip_len, uint8_t *frame_out);

#endif /* ENCAPSULATE_H */
