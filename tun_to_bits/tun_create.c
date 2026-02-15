/**
 * tun_create.c - 创建 TUN 设备（调用 tun_dev 的 tun_open）
 */

#include "tun_create.h"
#include "../include/tun_dev.h"

int tun_create(const char *name)
{
    return tun_open(name);
}
