/**
 * packet_read.c - 从 TUN 读一个 IP 包到缓冲区（调用 tun_dev 的 tun_read）
 */

#include "packet_read.h"
#include "../include/tun_dev.h"

int packet_read(int fd, uint8_t *buf, int maxlen)
{
    return tun_read(fd, buf, maxlen);
}
