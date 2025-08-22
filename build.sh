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
echo "🚀 构建完成!"

# 确定可执行文件路径
EXECUTABLE=""
EXEC_PATH=""
if [ -f "Touch_Controller_Arm" ]; then
    EXECUTABLE="Touch_Controller_Arm"
    EXEC_PATH="$(pwd)/Touch_Controller_Arm"
elif [ -f "Touch_Controller_Arm2" ]; then
    EXECUTABLE="Touch_Controller_Arm2"
    EXEC_PATH="$(pwd)/Touch_Controller_Arm2"
fi

if [ -n "$EXECUTABLE" ]; then
    echo "🎯 可执行文件位置: $EXEC_PATH"
    
    # 创建符号链接到根目录（如果需要）
    ROOT_EXECUTABLE="../$EXECUTABLE"
    if [ ! -f "$ROOT_EXECUTABLE" ] || [ "$EXEC_PATH" -nt "$ROOT_EXECUTABLE" ]; then
        ln -sf "build/$EXECUTABLE" "$ROOT_EXECUTABLE"
        echo "🔗 已创建符号链接: $ROOT_EXECUTABLE"
    fi
else
    echo "❌ 错误: 未找到可执行文件"
fi

# 检查OpenHaptics库连接状态
echo ""
echo "🔍 检查依赖库..."

# 检查程序文件
EXECUTABLE=""
if [ -f "Touch_Controller_Arm" ]; then
    EXECUTABLE="Touch_Controller_Arm"
elif [ -f "Touch_Controller_Arm2" ]; then
    EXECUTABLE="Touch_Controller_Arm2"
fi

if [ -n "$EXECUTABLE" ]; then
    echo "📋 可执行文件: $EXECUTABLE"
    
    # 检查OpenHaptics库
    if ldd "$EXECUTABLE" 2>/dev/null | grep -q "libHD"; then
        echo "✅ OpenHaptics库连接正常"
        HD_LIB_PATH=$(ldd "$EXECUTABLE" 2>/dev/null | grep "libHD" | awk '{print $3}')
        if [[ "$HD_LIB_PATH" == "/lib/"* ]] || [[ "$HD_LIB_PATH" == "/usr/lib/"* ]]; then
            echo "✅ 使用系统OpenHaptics库: $HD_LIB_PATH"
        else
            echo "⚠️  使用本地OpenHaptics库: $HD_LIB_PATH"
            echo "ℹ️  建议运行 ./setup_touch_environment.sh 安装到系统"
        fi
    else
        echo "❌ 错误: OpenHaptics库未正确连接"
        echo "💡 解决方案:"
        echo "   1. 运行: ./setup_touch_environment.sh"
        echo "   2. 然后重新编译: ./build_with_ros2.sh"
    fi
    
    # 检查ROS2库
    if [ -n "$ROS_DISTRO" ]; then
        if ldd "$EXECUTABLE" 2>/dev/null | grep -q "rclcpp"; then
            echo "✅ ROS2库连接正常"
        else
            echo "ℹ️  信息: ROS2库未连接（可能未编译ROS2支持）"
        fi
    fi
else
    echo "❌ 错误: 可执行文件未找到"
    echo "💡 请检查编译过程是否有错误"
fi

echo ""
echo "🚀 构建完成! 可以运行程序："
echo "   cd $(dirname $(pwd))"
if [ -n "$EXECUTABLE" ]; then
    echo "   ./$EXECUTABLE  # 或者"
    echo "   ./start_touch_controller.sh  # 推荐使用启动脚本"
    echo ""
    echo "💡 提示:"
    echo "   - 使用启动脚本可自动设置环境变量"
    echo "   - 如果touch设备无法加载，请运行 ./check_touch_environment.sh 检查环境"
else
    echo "   请检查编译错误并重新构建"
fi
