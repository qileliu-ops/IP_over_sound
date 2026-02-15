#!/bin/bash
# setup_tun.sh - 配置 Linux TUN 虚拟网卡 IP 与路由
# 用法: sudo ./scripts/setup_tun.sh [设备名] [本机IP]
# 示例: sudo ./scripts/setup_tun.sh tun0 10.0.0.1

DEV="${1:-tun0}"
IP="${2:-10.0.0.1}"

# 若 TUN 设备尚未创建，需先运行 ipo_sound 一次或使用 ip tuntap
if ! ip link show "$DEV" &>/dev/null; then
    echo "Device $DEV not found. Start ipo_sound first to create it, or run:"
    echo "  sudo ip tuntap add dev $DEV mode tun"
    exit 1
fi

sudo ip addr add "${IP}/24" dev "$DEV"
sudo ip link set "$DEV" up
echo "TUN $DEV up with $IP/24"
echo "Route: traffic to 10.0.0.0/24 goes via $DEV. Other machine can use 10.0.0.2/24."
