/**
 * tun_dev.c - Linux TUN 虚拟网卡实现
 *
 * 通过打开 /dev/net/tun 并 ioctl 配置为 TUN 设备，实现与内核交换 IP 包。
 * 需要 root 或 CAP_NET_ADMIN 权限。
 */

#include "tun_dev.h"
#include "common.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

/**
 * 打开 TUN 设备
 * name 如 "tun0"，实际会创建或绑定该名称的 TUN 接口
 */
int tun_open(const char *name)
{
    int fd;
    struct ifreq ifr;

    fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("tun_open: open /dev/net/tun");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;  /* TUN 模式，不包含额外包头 */
    if (name && name[0])
        strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);

    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("tun_open: ioctl TUNSETIFF");
        close(fd);
        return -1;
    }

    return fd;
}

int tun_read(int fd, uint8_t *buf, int maxlen)
{
    ssize_t n;

    if (fd < 0 || !buf || maxlen <= 0)
        return -1;

    n = read(fd, buf, (size_t)maxlen);
    if (n < 0) {
        perror("tun_read");
        return -1;
    }
    return (int)n;
}

int tun_write(int fd, const uint8_t *buf, int len)
{
    ssize_t n;

    if (fd < 0 || !buf || len <= 0)
        return -1;

    n = write(fd, buf, (size_t)len);
    if (n < 0) {
        perror("tun_write");
        return -1;
    }
    return (int)n;
}

void tun_close(int fd)
{
    if (fd >= 0)
        close(fd);
}
