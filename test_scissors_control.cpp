#include <iostream>
#include <string>
#include <sstream>
#include "ConfigLoader.h"

// 简化的ArmController类用于测试剪刀控制
class TestArmController {
public:
    bool isConnected() const { return true; }  // 模拟连接状态
    
    bool controlScissorsCustom(int port, int address, int data, int device) {
        std::ostringstream oss;
        oss << "{ \"command\": \"write_single_register\", \"port\": " << port 
            << ", \"address\": " << address << ", \"data\": " << data 
            << ", \"device\": " << device << " }";
        
        std::string command = oss.str();
        std::cout << "🔧 发送剪刀控制命令: " << command << std::endl;
        return true;
    }
};

// 简化的TouchArmController类用于测试剪刀控制
class TestTouchArmController {
private:
    TestArmController& m_armController;
    ConfigLoader* m_config;
    std::string m_deviceName;
    std::string m_endEffectorType;
    int m_scissorsModbusPort;
    int m_scissorsModbusAddress;
    int m_scissorsModbusDevice;
    int m_scissorsOpenData;
    int m_scissorsCloseData;
    bool m_scissorsState;
    
public:
    TestTouchArmController(TestArmController& armController, ConfigLoader* config, const std::string& deviceName)
        : m_armController(armController), m_config(config), m_deviceName(deviceName),
          m_endEffectorType("gripper"), m_scissorsModbusPort(1), m_scissorsModbusAddress(2),
          m_scissorsModbusDevice(1), m_scissorsOpenData(0), m_scissorsCloseData(1), m_scissorsState(false) {
        
        if (m_config) {
            std::string mappingPrefix = (deviceName == "Device1") ? "device1_mapping" : "device2_mapping";
            
            // 读取末端控制器类型
            m_endEffectorType = m_config->getString(mappingPrefix + ".end_effector_type", "gripper");
            
            if (m_endEffectorType == "scissors") {
                m_scissorsModbusPort = m_config->getInt(mappingPrefix + ".scissors_modbus_port", 1);
                m_scissorsModbusAddress = m_config->getInt(mappingPrefix + ".scissors_modbus_address", 2);
                m_scissorsModbusDevice = m_config->getInt(mappingPrefix + ".scissors_modbus_device", 1);
                m_scissorsOpenData = m_config->getInt(mappingPrefix + ".scissors_open_data", 0);
                m_scissorsCloseData = m_config->getInt(mappingPrefix + ".scissors_close_data", 1);
                
                std::cout << "✅ [" << m_deviceName << "] 剪刀控制已配置:" << std::endl;
                std::cout << "   - Modbus端口: " << m_scissorsModbusPort << std::endl;
                std::cout << "   - 寄存器地址: " << m_scissorsModbusAddress << std::endl;
                std::cout << "   - 设备号: " << m_scissorsModbusDevice << std::endl;
                std::cout << "   - 松开数据: " << m_scissorsOpenData << std::endl;
                std::cout << "   - 闭合数据: " << m_scissorsCloseData << std::endl;
            }
        }
    }
    
    void testScissorsControl() {
        if (m_endEffectorType == "scissors" && m_armController.isConnected()) {
            std::cout << "\n=== 测试剪刀控制 [" << m_deviceName << "] ===" << std::endl;
            
            // 测试松开剪刀
            std::cout << "测试1: 松开剪刀" << std::endl;
            m_armController.controlScissorsCustom(m_scissorsModbusPort, m_scissorsModbusAddress, 
                                                 m_scissorsOpenData, m_scissorsModbusDevice);
            
            // 测试闭合剪刀
            std::cout << "\n测试2: 闭合剪刀" << std::endl;
            m_armController.controlScissorsCustom(m_scissorsModbusPort, m_scissorsModbusAddress, 
                                                 m_scissorsCloseData, m_scissorsModbusDevice);
            
            std::cout << "\n=== 剪刀控制测试完成 ===" << std::endl;
        } else {
            std::cout << "❌ [" << m_deviceName << "] 剪刀控制不可用 (末端类型: " << m_endEffectorType << ")" << std::endl;
        }
    }
};

int main() {
    std::cout << "=== 剪刀控制功能测试 ===" << std::endl;
    
    // 加载配置文件
    ConfigLoader config("config.ini");
    if (!config.loadConfig()) {
        std::cout << "❌ 无法加载配置文件" << std::endl;
        return 1;
    }
    
    std::cout << "✅ 配置文件加载成功" << std::endl;
    
    // 创建测试控制器
    TestArmController armController1, armController2;
    TestTouchArmController touchController1(armController1, &config, "Device1");
    TestTouchArmController touchController2(armController2, &config, "Device2");
    
    // 测试设备1和设备2的剪刀控制
    touchController1.testScissorsControl();
    touchController2.testScissorsControl();
    
    return 0;
}
