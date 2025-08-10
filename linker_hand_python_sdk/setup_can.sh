#!/bin/bash

# LinkerHand CAN接口配置脚本
# 作者: AI Assistant
# 日期: 2025-07-03

echo "=== LinkerHand CAN接口配置脚本 ==="

# 检查是否以root权限运行
if [ "$EUID" -ne 0 ]; then
    echo "请使用sudo运行此脚本"
    exit 1
fi

# 加载CAN模块
echo "正在加载CAN模块..."
modprobe can
modprobe can_raw

# 检查can0接口是否存在
if ! ip link show can0 >/dev/null 2>&1; then
    echo "错误: can0接口不存在"
    echo "请检查CAN硬件是否正确连接"
    exit 1
fi

# 配置CAN接口
echo "正在配置CAN接口..."
ip link set can0 type can bitrate 1000000

# 启动CAN接口
echo "正在启动CAN接口..."
ip link set can0 up

# 检查接口状态
echo "检查CAN接口状态..."
ip link show can0

echo "CAN接口配置完成！"
echo "现在可以运行 ./start_gui.sh 启动GUI程序" 