#!/bin/bash

# Touch Controller Arm 一键启动脚本
# 自动设置环境并启动程序

echo "=== Touch Controller Arm 一键启动脚本 ==="

# 1. 设置环境变量
export GTDD_HOME=/usr
echo "✅ 环境变量已设置: GTDD_HOME=$GTDD_HOME"

# 2. 检查必要的依赖
echo ""
echo "=== 环境检查 ==="

# 检查可执行文件
if [ ! -f "./Touch_Controller_Arm2" ]; then
    echo "❌ 找不到可执行文件: ./Touch_Controller_Arm2"
    echo "正在尝试编译..."
    make
    if [ $? -ne 0 ]; then
        echo "❌ 编译失败"
        exit 1
    fi
fi

# 检查配置文件
if [ ! -f "./config.ini" ]; then
    echo "❌ 找不到配置文件: ./config.ini"
    exit 1
fi

# 检查OpenHaptics库
if [ ! -f "/usr/lib/libHD.so" ]; then
    echo "⚠️  OpenHaptics库未在系统路径中找到"
    echo "尝试运行setup_touch_environment.sh来配置环境"
    if [ -f "./setup_touch_environment.sh" ]; then
        ./setup_touch_environment.sh
    fi
fi

# 检查Touch设备
echo ""
echo "=== Touch设备检查 ==="
touch_devices=$(lsusb | grep -i "3D Systems\|Geomagic" | wc -l)
if [ $touch_devices -eq 0 ]; then
    echo "⚠️  未发现Touch设备，请检查USB连接"
else
    echo "✅ 发现 $touch_devices 个Touch设备"
fi

# 3. 显示当前配置
echo ""
echo "=== 当前配置 ==="
if [ -f "./config.ini" ]; then
    echo "Device1 末端控制器: $(grep "end_effector_type" config.ini | head -1 | cut -d'=' -f2 | tr -d ' ')"
    echo "Device2 末端控制器: $(grep "end_effector_type" config.ini | tail -1 | cut -d'=' -f2 | tr -d ' ')"
    echo "机械臂1 IP: $(grep "ip = 192.168.10.18" config.ini | cut -d'=' -f2 | tr -d ' ')"
    echo "机械臂2 IP: $(grep "ip = 192.168.10.19" config.ini | cut -d'=' -f2 | tr -d ' ')"
fi

# 4. 启动程序
echo ""
echo "=== 启动程序 ==="
echo "正在启动Touch Controller Arm..."
echo "程序启动后："
echo "  - 按设备按钮1控制机械臂位置和姿态"
echo "  - 按设备按钮2控制末端夹抓"
echo "  - 按'q'键退出程序"
echo ""

# 启动程序
./Touch_Controller_Arm2

echo ""
echo "程序已退出。"
