#include <iostream>
#includ    std::cou    std::cout << "  touch_rot_to_arm_rx = " << touch_rot_to_arm_rx << " (期望: 2)" << std::endl;
    std::cout << "  touch_rot_to_arm_ry = " << touch_rot_to_arm_ry << " (期望: 0)" << std::endl;
    std::cout << "  touch_rot_to_arm_rz = " << touch_rot_to_arm_rz << " (期望: 1)" << std::endl;< "  touch_pos_to_arm_x = " << touch_pos_to_arm_x << " (期望: 2)" << std::endl;
    std::cout << "  touch_pos_to_arm_y = " << touch_pos_to_arm_y << " (期望: 0)" << std::endl;
    std::cout << "  touch_pos_to_arm_z = " << touch_pos_to_arm_z << " (期望: 1)" << std::endl;string>
#include "ConfigLoader.h"

int main() {
    std::cout << "=== Device1 映射参数加载测试 ===" << std::endl;
    
    // 加载配置文件
    ConfigLoader config("config.ini");
    if (!config.loadConfig()) {
        std::cout << "❌ 无法加载配置文件" << std::endl;
        return 1;
    }
    
    std::cout << "✅ 配置文件加载成功" << std::endl;
    
    // 测试Device1映射参数
    std::string mappingPrefix = "device1_mapping";
    
    std::cout << "\n=== Device1 映射参数 ===" << std::endl;
    
    // 位置映射参数
    int touch_pos_to_arm_x = config.getInt(mappingPrefix + ".touch_pos_to_arm_x", -1);
    int touch_pos_to_arm_y = config.getInt(mappingPrefix + ".touch_pos_to_arm_y", -1);
    int touch_pos_to_arm_z = config.getInt(mappingPrefix + ".touch_pos_to_arm_z", -1);
    
    std::cout << "📍 位置映射参数:" << std::endl;
    std::cout << "   touch_pos_to_arm_x = " << touch_pos_to_arm_x << " (期望: 0)" << std::endl;
    std::cout << "   touch_pos_to_arm_y = " << touch_pos_to_arm_y << " (期望: 2)" << std::endl;
    std::cout << "   touch_pos_to_arm_z = " << touch_pos_to_arm_z << " (期望: 1)" << std::endl;
    
    // 旋转映射参数
    int touch_rot_to_arm_rx = config.getInt(mappingPrefix + ".touch_rot_to_arm_rx", -1);
    int touch_rot_to_arm_ry = config.getInt(mappingPrefix + ".touch_rot_to_arm_ry", -1);
    int touch_rot_to_arm_rz = config.getInt(mappingPrefix + ".touch_rot_to_arm_rz", -1);
    
    std::cout << "\n🔄 旋转映射参数:" << std::endl;
    std::cout << "   touch_rot_to_arm_rx = " << touch_rot_to_arm_rx << " (期望: 0)" << std::endl;
    std::cout << "   touch_rot_to_arm_ry = " << touch_rot_to_arm_ry << " (期望: 2)" << std::endl;
    std::cout << "   touch_rot_to_arm_rz = " << touch_rot_to_arm_rz << " (期望: 1)" << std::endl;
    
    // 验证结果
    std::cout << "\n=== 验证结果 ===" << std::endl;
    
    bool pos_x_ok = (touch_pos_to_arm_x == 2);
    bool pos_y_ok = (touch_pos_to_arm_y == 0);
    bool pos_z_ok = (touch_pos_to_arm_z == 1);
    bool rot_x_ok = (touch_rot_to_arm_rx == 2);
    bool rot_y_ok = (touch_rot_to_arm_ry == 0);
    bool rot_z_ok = (touch_rot_to_arm_rz == 1);
    
    std::cout << "touch_pos_to_arm_x: " << (pos_x_ok ? "✅ 正确" : "❌ 错误") << std::endl;
    std::cout << "touch_pos_to_arm_y: " << (pos_y_ok ? "✅ 正确" : "❌ 错误") << std::endl;
    std::cout << "touch_pos_to_arm_z: " << (pos_z_ok ? "✅ 正确" : "❌ 错误") << std::endl;
    std::cout << "touch_rot_to_arm_rx: " << (rot_x_ok ? "✅ 正确" : "❌ 错误") << std::endl;
    std::cout << "touch_rot_to_arm_ry: " << (rot_y_ok ? "✅ 正确" : "❌ 错误") << std::endl;
    std::cout << "touch_rot_to_arm_rz: " << (rot_z_ok ? "✅ 正确" : "❌ 错误") << std::endl;
    
    bool all_correct = pos_x_ok && pos_y_ok && pos_z_ok && rot_x_ok && rot_y_ok && rot_z_ok;
    
    std::cout << "\n=== 总结 ===" << std::endl;
    if (all_correct) {
        std::cout << "🎉 所有Device1映射参数都正确加载！" << std::endl;
    } else {
        std::cout << "⚠️  部分Device1映射参数加载不正确，请检查配置文件。" << std::endl;
    }
    
    // 额外显示其他相关参数
    std::string end_effector_type = config.getString(mappingPrefix + ".end_effector_type", "未知");
    bool use_dexterous_hand = config.getBool(mappingPrefix + ".use_dexterous_hand", false);
    
    std::cout << "\n=== 其他相关参数 ===" << std::endl;
    std::cout << "末端控制器类型: " << end_effector_type << std::endl;
    std::cout << "使用灵巧手: " << (use_dexterous_hand ? "是" : "否") << std::endl;
    
    return 0;
}
