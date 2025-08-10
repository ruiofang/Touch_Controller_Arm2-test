#include <iostream>
#include <string>
#include "ConfigLoader.h"

int main() {
    std::cout << "=== Device1 映射参数验证测试 ===" << std::endl;
    
    // 加载配置文件
    ConfigLoader config("config.ini");
    if (!config.loadConfig()) {
        std::cout << "❌ 无法加载配置文件" << std::endl;
        return 1;
    }
    
    std::cout << "✅ 配置文件加载成功" << std::endl;
    
    // 读取device1_mapping参数
    std::string mappingPrefix = "device1_mapping";
    
    std::cout << "\n=== Device1 当前映射参数 ===" << std::endl;
    
    // 位置轴映射
    int touch_pos_to_arm_x = config.getInt(mappingPrefix + ".touch_pos_to_arm_x", -1);
    int touch_pos_to_arm_y = config.getInt(mappingPrefix + ".touch_pos_to_arm_y", -1);
    int touch_pos_to_arm_z = config.getInt(mappingPrefix + ".touch_pos_to_arm_z", -1);
    
    std::cout << "位置轴映射 (当前 → 期望):" << std::endl;
    std::cout << "  touch_pos_to_arm_x = " << touch_pos_to_arm_x << " → 2" << std::endl;
    std::cout << "  touch_pos_to_arm_y = " << touch_pos_to_arm_y << " → 0" << std::endl;
    std::cout << "  touch_pos_to_arm_z = " << touch_pos_to_arm_z << " → 1" << std::endl;
    
    // 旋转轴映射
    int touch_rot_to_arm_rx = config.getInt(mappingPrefix + ".touch_rot_to_arm_rx", -1);
    int touch_rot_to_arm_ry = config.getInt(mappingPrefix + ".touch_rot_to_arm_ry", -1);
    int touch_rot_to_arm_rz = config.getInt(mappingPrefix + ".touch_rot_to_arm_rz", -1);
    
    std::cout << "\n旋转轴映射 (当前 → 期望):" << std::endl;
    std::cout << "  touch_rot_to_arm_rx = " << touch_rot_to_arm_rx << " → 2" << std::endl;
    std::cout << "  touch_rot_to_arm_ry = " << touch_rot_to_arm_ry << " → 0" << std::endl;
    std::cout << "  touch_rot_to_arm_rz = " << touch_rot_to_arm_rz << " → 1" << std::endl;
    
    // 验证结果
    std::cout << "\n=== 参数验证结果 ===" << std::endl;
    
    bool pos_x_ok = (touch_pos_to_arm_x == 2);
    bool pos_y_ok = (touch_pos_to_arm_y == 0);
    bool pos_z_ok = (touch_pos_to_arm_z == 1);
    bool rot_x_ok = (touch_rot_to_arm_rx == 2);
    bool rot_y_ok = (touch_rot_to_arm_ry == 0);
    bool rot_z_ok = (touch_rot_to_arm_rz == 1);
    
    std::cout << "位置轴映射检查:" << std::endl;
    std::cout << "  X轴映射: " << (pos_x_ok ? "✅ 正确" : "❌ 错误") << std::endl;
    std::cout << "  Y轴映射: " << (pos_y_ok ? "✅ 正确" : "❌ 错误") << std::endl;
    std::cout << "  Z轴映射: " << (pos_z_ok ? "✅ 正确" : "❌ 错误") << std::endl;
    
    std::cout << "\n旋转轴映射检查:" << std::endl;
    std::cout << "  RX轴映射: " << (rot_x_ok ? "✅ 正确" : "❌ 错误") << std::endl;
    std::cout << "  RY轴映射: " << (rot_y_ok ? "✅ 正确" : "❌ 错误") << std::endl;
    std::cout << "  RZ轴映射: " << (rot_z_ok ? "✅ 正确" : "❌ 错误") << std::endl;
    
    bool all_ok = pos_x_ok && pos_y_ok && pos_z_ok && rot_x_ok && rot_y_ok && rot_z_ok;
    
    std::cout << "\n🎯 总体结果: " << (all_ok ? "✅ 所有参数都正确加载" : "❌ 部分参数不正确") << std::endl;
    
    if (all_ok) {
        std::cout << "\n🎉 Device1的映射参数配置正确，参数更改已生效！" << std::endl;
    } else {
        std::cout << "\n⚠️  Device1的映射参数与期望值不匹配，请检查配置文件。" << std::endl;
    }
    
    return all_ok ? 0 : 1;
}
