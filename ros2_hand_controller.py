#!/usr/bin/env python3
"""
ROS2灵巧手控制节点
接收ROS2话题消息并控制对应的灵巧手
支持JSON格式的结构化消息
"""

import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import sys
import os
import json
import time

# 添加灵巧手SDK路径
sys.path.append(os.path.join(os.path.dirname(__file__), 'linker_hand_python_sdk'))

try:
    from LinkerHand.linker_hand_api import LinkerHandAPI
    LINKER_HAND_AVAILABLE = True
except ImportError as e:
    print(f"警告: 无法导入LinkerHand SDK: {e}")
    LINKER_HAND_AVAILABLE = False

class DexterousHandController(Node):
    def __init__(self):
        super().__init__('dexterous_hand_controller')
        
        # 初始化参数
        self.declare_parameter('hand_type', 'left')
        self.declare_parameter('hand_joint', 'L7')
        self.declare_parameter('can_interface', 'can0')
        self.declare_parameter('topic_name', '/dexterous_hand_1/command')
        
        # 获取参数
        self.hand_type = self.get_parameter('hand_type').value
        self.hand_joint = self.get_parameter('hand_joint').value  
        self.can_interface = self.get_parameter('can_interface').value
        self.topic_name = self.get_parameter('topic_name').value
        
        self.get_logger().info(f"初始化灵巧手控制器:")
        self.get_logger().info(f"  手型: {self.hand_type}")
        self.get_logger().info(f"  关节: {self.hand_joint}")
        self.get_logger().info(f"  CAN接口: {self.can_interface}")
        self.get_logger().info(f"  监听话题: {self.topic_name}")
        
        # 初始化灵巧手
        self.hand_api = None
        self.hand_instance = None
        self.initialized = False
        
        if LINKER_HAND_AVAILABLE:
            self.initialize_hand()
        else:
            self.get_logger().warning("LinkerHand SDK不可用，将模拟控制")
        
        # 创建订阅者
        self.subscription = self.create_subscription(
            String,
            self.topic_name,
            self.command_callback,
            10
        )
        
        self.get_logger().info("✅ ROS2灵巧手控制节点启动完成")
        self.get_logger().info("📋 支持的消息格式:")
        self.get_logger().info("   1. 简单字符串: 'ZQ', 'ZK'")
        self.get_logger().info("   2. JSON格式: {\"hand_type\": \"left\", \"action\": \"ZQ\", ...}")
    
    def initialize_hand(self):
        """初始化灵巧手"""
        try:
            # 创建LinkerHand API实例
            self.hand_api = LinkerHandAPI()
            
            # 初始化手部
            self.hand_instance = self.hand_api.initialize_hand(
                hand_type=self.hand_type,
                hand_joint=self.hand_joint,
                can_interface=self.can_interface
            )
            
            if self.hand_instance:
                self.initialized = True
                self.get_logger().info(f"✅ 灵巧手初始化成功: {self.hand_type} {self.hand_joint}")
                
                # 初始化为张开状态
                self.execute_action("ZK")
                
            else:
                self.get_logger().error("❌ 灵巧手初始化失败")
                
        except Exception as e:
            self.get_logger().error(f"❌ 灵巧手初始化异常: {e}")
    
    def command_callback(self, msg):
        """处理接收到的控制命令"""
        command = msg.data.strip()
        self.get_logger().info(f"📨 接收到消息: {command}")
        
        # 尝试解析JSON格式
        try:
            command_data = json.loads(command)
            self.handle_json_command(command_data)
        except json.JSONDecodeError:
            # 不是JSON格式，按简单字符串处理
            self.handle_simple_command(command)
    
    def handle_json_command(self, command_data):
        """处理JSON格式的命令"""
        self.get_logger().info("🔄 解析JSON命令...")
        
        # 验证必要字段
        required_fields = ['action']
        for field in required_fields:
            if field not in command_data:
                self.get_logger().error(f"❌ JSON命令缺少必要字段: {field}")
                return
        
        # 提取字段
        action = command_data.get('action')
        hand_type = command_data.get('hand_type', self.hand_type)
        hand_joint = command_data.get('hand_joint', self.hand_joint)
        timestamp = command_data.get('timestamp', int(time.time() * 1000))
        
        # 验证时间戳（可选，防止重放攻击）
        current_time = int(time.time() * 1000)
        if abs(current_time - timestamp) > 30000:  # 30秒容差
            self.get_logger().warning(f"⚠️  时间戳偏差较大: {abs(current_time - timestamp)}ms")
        
        # 验证手型匹配
        if hand_type != self.hand_type:
            self.get_logger().warning(f"⚠️  手型不匹配: 期望{self.hand_type}, 收到{hand_type}")
            # 可以选择忽略或执行
        
        # 验证关节匹配
        if hand_joint != self.hand_joint:
            self.get_logger().warning(f"⚠️  关节型号不匹配: 期望{self.hand_joint}, 收到{hand_joint}")
        
        # 显示解析的信息
        self.get_logger().info(f"📋 JSON命令解析:")
        self.get_logger().info(f"   手型: {hand_type}")
        self.get_logger().info(f"   关节: {hand_joint}")
        self.get_logger().info(f"   动作: {action}")
        self.get_logger().info(f"   时间戳: {timestamp}")
        
        # 执行动作
        self.execute_action(action)
    
    def handle_simple_command(self, command):
        """处理简单字符串命令"""
        self.get_logger().info("🔄 解析简单字符串命令...")
        
        # 解析命令
        if command in ['ZQ', 'ZK']:
            self.execute_action(command)
        elif command.startswith('custom:'):
            # 自定义动作命令格式: custom:action_name
            action_name = command.replace('custom:', '')
            self.execute_action(action_name)
        else:
            # 尝试直接执行命令
            self.execute_action(command)
    
    def execute_action(self, action):
        """执行灵巧手动作"""
        try:
            if self.initialized and self.hand_instance:
                # 使用真实的灵巧手
                success = self.hand_api.execute_action(self.hand_instance, action)
                if success:
                    self.get_logger().info(f"✅ 执行动作成功: {action}")
                    if action == 'ZQ':
                        self.get_logger().info("👊 灵巧手握拳")
                    elif action == 'ZK':
                        self.get_logger().info("👋 灵巧手张开")
                else:
                    self.get_logger().error(f"❌ 执行动作失败: {action}")
            else:
                # 模拟模式
                self.get_logger().info(f"🎭 模拟执行动作: {action}")
                if action == 'ZQ':
                    self.get_logger().info("👊 [模拟] 灵巧手握拳")
                elif action == 'ZK':
                    self.get_logger().info("👋 [模拟] 灵巧手张开")
                else:
                    self.get_logger().info(f"🎭 [模拟] 执行自定义动作: {action}")
                    
        except Exception as e:
            self.get_logger().error(f"❌ 执行动作异常: {action}, 错误: {e}")
    
    def destroy_node(self):
        """节点销毁时的清理工作"""
        if self.initialized and self.hand_api:
            try:
                # 在退出前将手张开
                self.execute_action("ZK")
                self.hand_api.cleanup()
                self.get_logger().info("🧹 灵巧手资源清理完成")
            except Exception as e:
                self.get_logger().error(f"❌ 资源清理异常: {e}")
        
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    
    try:
        node = DexterousHandController()
        rclpy.spin(node)
    except KeyboardInterrupt:
        print("\n🛑 接收到中断信号，正在退出...")
    except Exception as e:
        print(f"❌ 节点运行异常: {e}")
    finally:
        try:
            node.destroy_node()
        except:
            pass
        rclpy.shutdown()
        print("👋 ROS2灵巧手控制节点已退出")

if __name__ == '__main__':
    main()
