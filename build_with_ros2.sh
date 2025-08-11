#!/bin/bash

# 灵巧手控制程序构建脚本
# 支持ROS2集成的构建

set -e  # 出错时停止

echo "=== 灵巧手控制程序构建脚本 ==="

# 检查构建目录
if [ ! -d "build" ]; then
    echo "📁 创建构建目录..."
    mkdir build
fi

cd build

echo "🔧 配置CMake..."

# 检查ROS2环境
if [ -n "$ROS_DISTRO" ]; then
    echo "✅ 检测到ROS2环境: $ROS_DISTRO"
    echo "🚀 启用ROS2支持构建"
    
    # 使用colcon构建
    if command -v colcon &> /dev/null; then
        echo "📦 使用colcon构建工具"
        cd ..
        colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release
        echo "✅ colcon构建完成"
        echo "🎯 ROS2可执行文件位置: install/touch_controller_arm/lib/touch_controller_arm/Touch_Controller_Arm2"
        
        # 创建符合预期命名的符号链接
        if [ -f "install/touch_controller_arm/lib/touch_controller_arm/Touch_Controller_Arm2" ]; then
            echo "🔗 创建符号链接: Touch_Controller_Arm2"
            ln -sf "install/touch_controller_arm/lib/touch_controller_arm/Touch_Controller_Arm2" "Touch_Controller_Arm2"
            echo "✅ 现在可以使用 ./Touch_Controller_Arm2 启动程序"
        else
            echo "⚠️  可执行文件未找到，请检查构建日志"
        fi
        exit 0
    fi
fi

echo "⚠️  未检测到ROS2环境或colcon，使用传统cmake构建"

# 传统CMake构建
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "🔨 开始编译..."
make -j$(nproc)

echo ""
echo "✅ 构建完成!"
echo "🎯 可执行文件位置: $(pwd)/Touch_Controller_Arm"

# 检查OpenHaptics库
echo ""
echo "🔍 检查依赖库..."
if ldd Touch_Controller_Arm 2>/dev/null | grep -q "libHD"; then
    echo "✅ OpenHaptics库连接正常"
else
    echo "⚠️  警告: OpenHaptics库可能未正确连接"
fi

# 检查ROS2库
if [ -n "$ROS_DISTRO" ]; then
    if ldd Touch_Controller_Arm 2>/dev/null | grep -q "rclcpp"; then
        echo "✅ ROS2库连接正常"
    else
        echo "ℹ️  信息: ROS2库未连接（可能未编译ROS2支持）"
    fi
fi

echo ""
echo "🚀 构建完成! 可以运行程序："
echo "   cd $(dirname $(pwd))"
echo "   ./build/Touch_Controller_Arm"
