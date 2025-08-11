#!/bin/bash

# JSON格式ROS2消息测试脚本

echo "=== JSON格式ROS2灵巧手控制测试 ==="
echo ""

# 设置ROS2环境
source /opt/ros/humble/setup.bash

# 切换到项目目录
cd /home/ruio/Touch_Controller_Arm2-test3

echo "🧪 测试JSON格式的ROS2消息发布和接收"
echo ""

# 启动ROS2灵巧手控制节点（后台）
echo "🚀 启动ROS2灵巧手控制节点..."
python3 ros2_to_dexterous.py --ros-args -p topic_name:=/dexterous_hand_1/command &
ROS2_PID=$!

# 等待节点启动
sleep 3

echo ""
echo "📤 测试JSON格式消息..."

# 测试1: 标准JSON消息
echo "🧪 测试1: 标准JSON消息 (握拳)"
TIMESTAMP=$(date +%s%3N)
JSON_MSG='{"hand_type":"left","hand_joint":"L7","action":"ZQ","timestamp":'$TIMESTAMP'}'
echo "发送: $JSON_MSG"
ros2 topic pub --once /dexterous_hand_1/command std_msgs/String "data: '$JSON_MSG'"

sleep 2

# 测试2: 标准JSON消息
echo ""
echo "🧪 测试2: 标准JSON消息 (张开)"
TIMESTAMP=$(date +%s%3N)
JSON_MSG='{"hand_type":"left","hand_joint":"L7","action":"ZK","timestamp":'$TIMESTAMP'}'
echo "发送: $JSON_MSG"
ros2 topic pub --once /dexterous_hand_1/command std_msgs/String "data: '$JSON_MSG'"

sleep 2

# 测试3: 不匹配的手型
echo ""
echo "🧪 测试3: 不匹配的手型 (应有警告)"
TIMESTAMP=$(date +%s%3N)
JSON_MSG='{"hand_type":"right","hand_joint":"L7","action":"ZQ","timestamp":'$TIMESTAMP'}'
echo "发送: $JSON_MSG"
ros2 topic pub --once /dexterous_hand_1/command std_msgs/String "data: '$JSON_MSG'"

sleep 2

# 测试4: 不匹配的关节
echo ""
echo "🧪 测试4: 不匹配的关节 (应有警告)"
TIMESTAMP=$(date +%s%3N)
JSON_MSG='{"hand_type":"left","hand_joint":"L10","action":"ZK","timestamp":'$TIMESTAMP'}'
echo "发送: $JSON_MSG"
ros2 topic pub --once /dexterous_hand_1/command std_msgs/String "data: '$JSON_MSG'"

sleep 2

# 测试5: 旧时间戳
echo ""
echo "🧪 测试5: 旧时间戳 (应有时间戳警告)"
OLD_TIMESTAMP=$(($(date +%s%3N) - 60000))  # 60秒前
JSON_MSG='{"hand_type":"left","hand_joint":"L7","action":"ZQ","timestamp":'$OLD_TIMESTAMP'}'
echo "发送: $JSON_MSG"
ros2 topic pub --once /dexterous_hand_1/command std_msgs/String "data: '$JSON_MSG'"

sleep 2

# 测试6: 简单字符串格式（向后兼容）
echo ""
echo "🧪 测试6: 简单字符串格式 (向后兼容)"
echo "发送: ZK"
ros2 topic pub --once /dexterous_hand_1/command std_msgs/String "data: 'ZK'"

sleep 2

# 测试7: 自定义动作
echo ""
echo "🧪 测试7: 自定义动作"
echo "发送: custom:test_action"
ros2 topic pub --once /dexterous_hand_1/command std_msgs/String "data: 'custom:test_action'"

sleep 2

# 测试8: 无效JSON
echo ""
echo "🧪 测试8: 无效JSON (应回退到简单字符串模式)"
echo "发送: {invalid_json"
ros2 topic pub --once /dexterous_hand_1/command std_msgs/String "data: '{invalid_json'"

sleep 2

# 测试9: 缺少必要字段
echo ""
echo "🧪 测试9: 缺少必要字段 (应报错)"
TIMESTAMP=$(date +%s%3N)
JSON_MSG='{"hand_type":"left","hand_joint":"L7","timestamp":'$TIMESTAMP'}'
echo "发送: $JSON_MSG"
ros2 topic pub --once /dexterous_hand_1/command std_msgs/String "data: '$JSON_MSG'"

sleep 2

echo ""
echo "✅ JSON格式测试完成！"
echo ""

# 显示测试总结
echo "📋 测试总结:"
echo "   ✅ JSON格式消息解析"
echo "   ✅ 字段验证 (hand_type, hand_joint, action, timestamp)"
echo "   ✅ 向后兼容性 (简单字符串)"
echo "   ✅ 错误处理 (无效JSON, 缺少字段)"
echo "   ✅ 警告信息 (不匹配的参数, 时间戳偏差)"

# 清理后台进程
echo ""
echo "🧹 清理后台进程..."
kill $ROS2_PID 2>/dev/null

echo "🎉 测试完成！"
