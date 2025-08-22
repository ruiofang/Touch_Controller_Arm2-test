#!/bin/bash

# ROS2灵巧手控制节点启动脚本

echo "=== ROS2灵巧手控制节点启动器 ==="
echo "📡 此节点将监听ROS2话题并控制灵巧手"
echo ""

# 切换到项目目录
cd /home/ruio/Touch_Controller_Arm2-test3

# 检查ROS2环境
if [ -z "$ROS_DISTRO" ]; then
    echo "⚠️  警告: ROS2环境未设置，正在尝试设置..."
    source /opt/ros/humble/setup.bash
    if [ -z "$ROS_DISTRO" ]; then
        echo "❌ 无法设置ROS2环境，请检查ROS2安装"
        exit 1
    fi
fi

echo "✅ ROS2环境: $ROS_DISTRO"

# 检查Python依赖
echo "🔍 检查Python依赖..."
python3 -c "import rclpy" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "❌ rclpy未安装，请安装: sudo apt install python3-rclpy"
    exit 1
fi

python3 -c "from std_msgs.msg import String" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "❌ std_msgs未安装，请安装: sudo apt install ros-humble-std-msgs"
    exit 1
fi

echo "✅ Python依赖检查完成"

# 设置参数
HAND_TYPE=${1:-"left"}
HAND_JOINT=${2:-"L7"}
CAN_INTERFACE=${3:-"can0"}
TOPIC_NAME=${4:-"/dexterous_hand_1/command"}

echo ""
echo "🤖 启动参数:"
echo "   手型: $HAND_TYPE"
echo "   关节: $HAND_JOINT"
echo "   CAN接口: $CAN_INTERFACE"
echo "   监听话题: $TOPIC_NAME"
echo ""

# 启动节点
echo "🚀 启动ROS2灵巧手控制节点..."
echo "📡 监听话题: $TOPIC_NAME"
echo "🛑 按 Ctrl+C 退出"
echo ""

# 运行节点
python3 ros2_hand_controller.py \
    --ros-args \
    -p hand_type:=$HAND_TYPE \
    -p hand_joint:=$HAND_JOINT \
    -p can_interface:=$CAN_INTERFACE \
    -p topic_name:=$TOPIC_NAME

echo ""
echo "👋 ROS2灵巧手控制节点已退出"
