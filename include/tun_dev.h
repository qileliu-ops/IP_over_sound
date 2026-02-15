/**
 * tun_dev.h - TUN 虚拟网卡操作接口
 *
 * 负责打开/读/写 Linux TUN 设备，与内核交换 IP 包。
 * 内核把要发出的包写入 TUN，本模块读出；本模块写入 TUN 的包会被内核当作“网卡收到”。
 */

#ifndef TUN_DEV_H
#define TUN_DEV_H

#include <stdint.h>

/**
 * 打开并配置 TUN 设备
 * @param name 设备名，如 "tun0"
 * @return     成功返回文件描述符 (>=0)，失败返回 -1
 */
int tun_open(const char *name);

/**
 * 从 TUN 读取一个 IP 包（阻塞）
 * @param fd     tun_open 返回的描述符
 * @param buf    缓冲区，需至少 MAX_FRAME_PAYLOAD 字节
 * @param maxlen 缓冲区大小
 * @return       成功返回读取的字节数，失败返回 -1
 */
int tun_read(int fd, uint8_t *buf, int maxlen);

/**
 * 向 TUN 写入一个 IP 包（注入到内核）
 * @param fd   tun_open 返回的描述符
 * @param buf  IP 包数据
 * @param len  包长度
 * @return     成功返回写入字节数，失败返回 -1
 */
int tun_write(int fd, const uint8_t *buf, int len);

/**
 * 关闭 TUN 设备
 * @param fd tun_open 返回的描述符
 */
void tun_close(int fd);

#endif /* TUN_DEV_H */
