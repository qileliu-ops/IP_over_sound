/**
 * packet_read.h - 从 TUN 读一个 IP 包到缓冲区
 */

#ifndef PACKET_READ_H
#define PACKET_READ_H

#include <stdint.h>

/**
 * 从 TUN 设备读一个 IP 包（阻塞）
 * @param fd     TUN 文件描述符（由 tun_create 返回）
 * @param buf    输出缓冲区
 * @param maxlen 缓冲区最大字节数
 * @return       成功返回读到的字节数，失败返回 -1
 */
int packet_read(int fd, uint8_t *buf, int maxlen);

#endif /* PACKET_READ_H */
