#!/bin/bash
# Touch设备诊断脚本

echo "=== Touch设备诊断 ==="
echo

# 检查USB设备
echo "1. USB设备检查:"
echo "查找Geomagic/3D Systems设备..."
lsusb | grep -i "geomagic\|3d systems\|sensable" || echo "未找到Geomagic设备"
echo

# 检查所有USB设备（寻找可能的Touch设备）
echo "2. 所有USB设备:"
lsusb
echo

# 检查设备文件
echo "3. 设备文件检查:"
if [ -d "/dev/input" ]; then
    echo "输入设备:"
    ls -la /dev/input/ | grep -E "(event|js|mouse)"
else
    echo "/dev/input 目录不存在"
fi
echo

# 检查系统日志中的USB相关信息
echo "4. 最近的USB日志 (最后20行):"
sudo dmesg | grep -i usb | tail -20
echo

# 检查OpenHaptics进程
echo "5. OpenHaptics进程检查:"
ps aux | grep -i phantom || echo "未找到OpenHaptics相关进程"
echo

# 检查环境变量
echo "6. 环境变量:"
echo "GTDD_HOME: $GTDD_HOME"
echo

# 检查配置目录
echo "7. 配置目录检查:"
if [ -d "/usr/config" ]; then
    echo "/usr/config 存在"
    ls -la /usr/config/ 2>/dev/null || echo "目录为空或无法访问"
else
    echo "/usr/config 不存在"
fi

if [ -d "$HOME/.3dsystems" ]; then
    echo "$HOME/.3dsystems 存在"
    ls -la "$HOME/.3dsystems/" 2>/dev/null || echo "目录为空或无法访问"
else
    echo "$HOME/.3dsystems 不存在"
fi
echo

echo "=== 诊断完成 ==="
echo
echo "如果没有找到Touch设备，请:"
echo "1. 检查USB线缆连接"
echo "2. 尝试重新插拔设备"
echo "3. 检查设备是否正常供电"
echo "4. 运行: sudo udevadm monitor 然后插拔设备查看系统识别情况"
