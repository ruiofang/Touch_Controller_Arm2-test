#include <iostream>
#include <string>
#include "ConfigLoader.h"

int main() {
    std::cout << "=== 完整的Device1映射参数验证 ===" << std::endl;
    
    // 加载配置文件
    ConfigLoader g_config("config.ini");
    if (!g_config.loadConfig()) {
        std::cout << "❌ 无法加载配置文件" << std::endl;
        return 1;
    }
    
    std::cout << "✅ 配置文件加载成功\n" << std::endl;
    
    // 显示当前坐标映射配置 (模拟主程序中的代码)
    std::cout << "=== 当前坐标映射配置 ===" << std::endl;
    
    // 显示设备1配置
    std::cout << "设备1 (device1_mapping):" << std::endl;
    
    int pos_x = g_config.getInt("device1_mapping.touch_pos_to_arm_x", 2);
    int pos_y = g_config.getInt("device1_mapping.touch_pos_to_arm_y", 0);
    int pos_z = g_config.getInt("device1_mapping.touch_pos_to_arm_z", 1);
    
    std::cout << "  位置映射: [" 
              << pos_x << "→X, "
              << pos_y << "→Y, "
              << pos_z << "→Z]" << std::endl;
              
    int rot_x = g_config.getInt("device1_mapping.touch_rot_to_arm_rx", 2);
    int rot_y = g_config.getInt("device1_mapping.touch_rot_to_arm_ry", 0);
    int rot_z = g_config.getInt("device1_mapping.touch_rot_to_arm_rz", 1);
    
    std::cout << "  姿态映射: [" 
              << rot_x << "→RX, "
              << rot_y << "→RY, "
              << rot_z << "→RZ]" << std::endl;
    std::cout << "  符号调整: [" 
              << g_config.getInt("device1_mapping.arm_x_sign", -1) << ", "
              << g_config.getInt("device1_mapping.arm_y_sign", -1) << ", "
              << g_config.getInt("device1_mapping.arm_z_sign", 1) << ", "
              << g_config.getInt("device1_mapping.arm_rx_sign", -1) << ", "
              << g_config.getInt("device1_mapping.arm_ry_sign", -1) << ", "
              << g_config.getInt("device1_mapping.arm_rz_sign", 1) << "]" << std::endl;
    
    std::cout << "\n=== 期望值对比 ===" << std::endl;
    std::cout << "期望的Device1映射配置:" << std::endl;
    std::cout << "  位置映射: [0→X, 2→Y, 1→Z]" << std::endl;
    std::cout << "  姿态映射: [0→RX, 2→RY, 1→RZ]" << std::endl;
    
    std::cout << "\n=== 验证结果 ===" << std::endl;
    
    bool pos_correct = (pos_x == 0 && pos_y == 2 && pos_z == 1);
    bool rot_correct = (rot_x == 0 && rot_y == 2 && rot_z == 1);
    
    std::cout << "位置映射: " << (pos_correct ? "✅ 正确" : "❌ 错误") << std::endl;
    std::cout << "  实际: [" << pos_x << "→X, " << pos_y << "→Y, " << pos_z << "→Z]" << std::endl;
    std::cout << "  期望: [0→X, 2→Y, 1→Z]" << std::endl;
    
    std::cout << "\n姿态映射: " << (rot_correct ? "✅ 正确" : "❌ 错误") << std::endl;
    std::cout << "  实际: [" << rot_x << "→RX, " << rot_y << "→RY, " << rot_z << "→RZ]" << std::endl;
    std::cout << "  期望: [0→RX, 2→RY, 1→RZ]" << std::endl;
    
    if (pos_correct && rot_correct) {
        std::cout << "\n🎉 Device1的所有映射参数都正确加载！" << std::endl;
    } else {
        std::cout << "\n⚠️  Device1的映射参数与期望值不匹配！" << std::endl;
    }
    
    std::cout << "\n说明:" << std::endl;
    std::cout << "  位置映射: 触觉设备轴索引(0=X,1=Y,2=Z) → 机械臂轴" << std::endl;
    std::cout << "  姿态映射: 触觉设备旋转轴索引(0=RX,1=RY,2=RZ) → 机械臂旋转轴" << std::endl;
    
    return 0;
}
