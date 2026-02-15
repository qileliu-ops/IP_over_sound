/**
 * tun_create.h - 创建 TUN 设备
 * 封装项目 tun_dev 的 tun_open，在本模块中作为独立“创建 TUN”一步。
 */

#ifndef TUN_CREATE_H
#define TUN_CREATE_H

/**
 * 创建并绑定 TUN 设备
 * @param name 设备名，如 "tun0"
 * @return     成功返回文件描述符 (>=0)，失败返回 -1
 */
int tun_create(const char *name);

#endif /* TUN_CREATE_H */
