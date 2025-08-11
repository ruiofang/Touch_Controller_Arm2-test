# 灵巧手控制ROS2集成说明

## 概述

本项目已增加ROS2话题支持，允许通过ROS2话题控制灵巧手，而无需直接使用CAN总线。当配置为使用ROS2时，程序将发布ROS2话题消息，而不是直接控制CAN。

## 新增功能

### 1. 配置文件选项

在`config.ini`中，每个设备的`device*_mapping`节新增了以下选项：

```ini
[device1_mapping]
use_ros2 = false  # 是否使用ROS2话题控制灵巧手（不使用CAN直接控制）
ros2_topic_name = /dexterous_hand_1/command  # ROS2话题名称

[device2_mapping]
use_ros2 = false  # 是否使用ROS2话题控制灵巧手（不使用CAN直接控制）
ros2_topic_name = /dexterous_hand_2/command  # ROS2话题名称
```

### 2. ROS2消息格式

当启用ROS2模式时，程序会发布`std_msgs/String`类型的消息，消息内容为JSON格式：

```json
{
  "hand_type": "left",
  "hand_joint": "L7",
  "action": "ZQ",
  "timestamp": 1691234567890
}
```

消息字段说明：
- `hand_type`: 手型（left/right）
- `hand_joint`: 关节数（L7/L10/L20/L25）
- `action`: 动作名称（如"ZQ"表示抓取，"张开"表示松开）
- `timestamp`: 时间戳（毫秒）

### 3. 编译支持

项目使用条件编译来支持ROS2：

- 如果系统安装了ROS2（检测到`rclcpp`和`std_msgs`包），则编译时启用ROS2支持
- 如果未找到ROS2，则禁用ROS2功能，但不影响其他功能的正常使用

## 使用方法

### 1. 启用ROS2模式

1. 编辑`config.ini`文件
2. 将对应设备的`use_ros2`设置为`true`
3. 设置合适的`ros2_topic_name`
4. 重启程序

示例配置：
```ini
[device1_mapping]
use_ros2 = true
ros2_topic_name = /left_hand/command
hand_type = left
hand_joint = L7
# 其他参数...

[device2_mapping]
use_ros2 = true
ros2_topic_name = /right_hand/command
hand_type = right
hand_joint = L7
# 其他参数...
```

### 2. 订阅ROS2话题

在ROS2环境中，可以通过以下命令监听灵巧手控制消息：

```bash
# 监听设备1的控制消息
ros2 topic echo /dexterous_hand_1/command

# 监听设备2的控制消息
ros2 topic echo /dexterous_hand_2/command
```

### 3. 编译项目

确保已安装ROS2环境，然后编译：

```bash
# 如果使用colcon构建工具
colcon build

# 如果使用传统cmake
mkdir build && cd build
cmake ..
make
```

## 工作模式

### CAN直接控制模式（默认）
- `use_ros2 = false`
- 程序直接通过CAN总线控制灵巧手
- 需要加载Python SDK和CAN接口
- 适用于直接硬件控制

### ROS2话题模式
- `use_ros2 = true`
- 程序发布ROS2话题消息
- 不加载CAN接口，不直接控制硬件
- 适用于分布式系统，灵巧手控制由其他ROS2节点处理

## 注意事项

1. **模式互斥**：每个设备只能选择一种控制模式，不能同时使用CAN和ROS2
2. **依赖关系**：ROS2模式不需要Python SDK和CAN接口，减少了依赖
3. **话题名称**：确保ROS2话题名称在系统中唯一，避免冲突
4. **时间同步**：消息包含时间戳，便于调试和性能分析
5. **错误处理**：如果ROS2初始化失败，程序会回退到错误状态但不会崩溃

## 扩展建议

1. **自定义消息类型**：可以定义专门的灵巧手控制消息类型，而不是使用String
2. **反馈机制**：增加灵巧手状态反馈的ROS2话题
3. **参数服务**：使用ROS2参数服务动态调整控制参数
4. **服务接口**：提供ROS2服务接口用于同步控制操作

## 故障排除

### 编译错误
- 确保已安装ROS2环境
- 检查`rclcpp`和`std_msgs`包是否可用
- 验证CMakeLists.txt中的ROS2包发现逻辑

### 运行时错误
- 检查ROS2环境变量是否正确设置
- 确认话题名称没有冲突
- 验证ROS2 daemon是否运行

### 配置问题
- 检查config.ini文件语法
- 确认布尔值格式正确（true/false）
- 验证话题名称格式符合ROS2规范
