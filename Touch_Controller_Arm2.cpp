/*****************************************************************************
 双触觉设备双机械臂控制程序
 
 功能：
 - 同时使用两个触觉设备分别控制两个机械臂
 - Device1控制机械臂1，Device2控制机械臂2
 - 每个设备独立按钮控制对应机械臂
 - 使用TCP连接和JSON协议与机械臂通信
 
 坐标映射：
 - 支持通过config.ini配置文件自定义坐标映射关系
 - 默认位置映射：触觉设备[Z,X,Y] → 机械臂[X,Y,Z]
 - 默认姿态映射：触觉设备[RZ,RX,RY] → 机械臂[-RX,-RY,RZ]
 - 设备1使用device1_mapping节配置映射参数
 - 设备2使用device2_mapping节配置映射参数
 - 每个设备可独立配置映射索引和符号调整
 
 操作说明：
 - 设备1按钮1：控制机械臂1位置姿态
 - 设备1按钮2：控制机械臂1夹抓（力控抓取/释放）
 - 设备2按钮1：控制机械臂2位置姿态  
 - 设备2按钮2：控制机械臂2夹抓（力控抓取/释放）
 - 键盘 'q' 退出程序
 - 键盘 '1'/'2' 选择调整设备1或设备2的参数
 - 键盘 '+'/'-' 调整当前选择设备的位置映射系数
 - 键盘 '['/']' 调整当前选择设备的姿态映射系数
*****************************************************************************/

#include <stdio.h>
#include <iostream>
#include <array>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <exception>
#include <chrono>

#if defined(WIN32)
# include <windows.h>
# include <conio.h>
# include <winsock2.h>
# include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
# include <unistd.h>
extern "C" {
# include "conio.h"
}
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netinet/in.h>
#endif

#define linux 1  // 确保定义linux宏
#include <HD/hd.h>
#include <HDU/hduError.h>
#include <HDU/hduVector.h>
#include "ConfigLoader.h"

// 添加Python支持的头文件
#include <Python.h>
#include <cstdlib>

// 添加ROS2支持的头文件
#ifdef USE_ROS2
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <memory>
#endif

// 灵巧手控制类
class DexterousHandController {
private:
    std::string m_handType;      // left 或 right
    std::string m_handJoint;     // L7, L10, L20, L25
    std::string m_canInterface;  // can0, can1 等
    std::string m_graspAction;   // 抓取动作名称
    std::string m_releaseAction; // 松开动作名称
    bool m_handOpen;             // 当前手势状态，true为张开，false为握拳
    bool m_pythonInitialized;    // Python环境是否已初始化
    PyObject* m_handModule;      // Python模块对象
    PyObject* m_handInstance;    // 灵巧手实例对象
    PyObject* m_yamlLoader;      // YAML加载器对象
    
    // ROS2相关成员
    bool m_useRos2;              // 是否使用ROS2
    std::string m_ros2TopicName; // ROS2话题名称
#ifdef USE_ROS2
    std::shared_ptr<rclcpp::Node> m_ros2Node;        // ROS2节点
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr m_ros2Publisher;  // ROS2发布者
#endif
    
public:
    DexterousHandController(const std::string& handType = "left", 
                          const std::string& handJoint = "L7",
                          const std::string& canInterface = "can0",
                          const std::string& graspAction = "ZQ",
                          const std::string& releaseAction = "张开",
                          bool useRos2 = false,
                          const std::string& ros2TopicName = "/dexterous_hand/command")
        : m_handType(handType), m_handJoint(handJoint), m_canInterface(canInterface),
          m_graspAction(graspAction), m_releaseAction(releaseAction),
          m_handOpen(true), m_pythonInitialized(false), m_handModule(nullptr), 
          m_handInstance(nullptr), m_yamlLoader(nullptr), m_useRos2(useRos2), 
          m_ros2TopicName(ros2TopicName) {
#ifdef USE_ROS2
        if (m_useRos2) {
            // 初始化ROS2
            if (!rclcpp::ok()) {
                rclcpp::init(0, nullptr);
            }
        }
#endif
    }
    
    ~DexterousHandController() {
        cleanup();
    }
    
    bool initialize() {
        if (m_useRos2) {
            return initializeRos2();
        } else {
            return initializePython();
        }
    }
    
    void openHand() {
        if (m_useRos2) {
#ifdef USE_ROS2
            publishRos2Command(m_releaseAction);
#else
            std::cerr << "❌ ROS2支持未编译" << std::endl;
#endif
        } else {
            openHandPython();
        }
        m_handOpen = true;
        std::cout << "👋 灵巧手张开 (" << m_releaseAction << ")" << std::endl;
    }
    
    void closeHand() {
        if (m_useRos2) {
#ifdef USE_ROS2
            publishRos2Command(m_graspAction);
#else
            std::cerr << "❌ ROS2支持未编译" << std::endl;
#endif
        } else {
            closeHandPython();
        }
        m_handOpen = false;
        std::cout << "✊ 灵巧手握拳 (" << m_graspAction << ")" << std::endl;
    }
    
    void toggleHand() {
        if (m_handOpen) {
            closeHand();
        } else {
            openHand();
        }
    }
    
    bool isHandOpen() const {
        return m_handOpen;
    }
    
    bool isInitialized() const {
        if (m_useRos2) {
#ifdef USE_ROS2
            return m_ros2Node && m_ros2Publisher;
#else
            return false;
#endif
        } else {
            return m_pythonInitialized;
        }
    }
    
private:
    bool initializeRos2() {
#ifdef USE_ROS2
        try {
            // 创建ROS2节点
            std::string nodeName = "dexterous_hand_controller_" + m_handType;
            m_ros2Node = rclcpp::Node::make_shared(nodeName);
            
            // 创建发布者
            m_ros2Publisher = m_ros2Node->create_publisher<std_msgs::msg::String>(
                m_ros2TopicName, 10);
            
            std::cout << "✅ ROS2灵巧手控制器初始化成功: " << m_handType << " " 
                      << m_handJoint << " (话题: " << m_ros2TopicName << ")" << std::endl;
            
            // 初始化为张开状态
            openHand();
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "❌ ROS2灵巧手初始化异常: " << e.what() << std::endl;
            return false;
        }
#else
        std::cerr << "❌ 程序未编译ROS2支持，无法使用ROS2模式" << std::endl;
        return false;
#endif
    }
    
    bool initializePython() {
        try {
            // 初始化Python环境
            if (!Py_IsInitialized()) {
                Py_Initialize();
                if (!Py_IsInitialized()) {
                    std::cerr << "❌ Python环境初始化失败" << std::endl;
                    return false;
                }
            }
            
            // 添加linker_hand_python_sdk路径到Python路径
            PyRun_SimpleString("import sys");
            std::string sdkPath = "sys.path.append('linker_hand_python_sdk')";
            PyRun_SimpleString(sdkPath.c_str());
            
            // 导入LinkerHand模块
            m_handModule = PyImport_ImportModule("LinkerHand.linker_hand_api");
            if (!m_handModule) {
                PyErr_Print();
                std::cerr << "❌ 无法导入LinkerHand模块" << std::endl;
                return false;
            }
            
            // 获取LinkerHandApi类
            PyObject* handClass = PyObject_GetAttrString(m_handModule, "LinkerHandApi");
            if (!handClass) {
                PyErr_Print();
                std::cerr << "❌ 无法获取LinkerHandApi类" << std::endl;
                return false;
            }
            
            // 创建LinkerHandApi实例
            PyObject* args = PyTuple_New(0);
            PyObject* kwargs = PyDict_New();
            PyDict_SetItemString(kwargs, "hand_type", PyUnicode_FromString(m_handType.c_str()));
            PyDict_SetItemString(kwargs, "hand_joint", PyUnicode_FromString(m_handJoint.c_str()));
            PyDict_SetItemString(kwargs, "can", PyUnicode_FromString(m_canInterface.c_str()));
            
            m_handInstance = PyObject_Call(handClass, args, kwargs);
            
            Py_DECREF(args);
            Py_DECREF(kwargs);
            Py_DECREF(handClass);
            
            if (!m_handInstance) {
                PyErr_Print();
                std::cerr << "❌ 无法创建LinkerHandApi实例" << std::endl;
                return false;
            }
            
            m_pythonInitialized = true;
            std::cout << "✅ 灵巧手控制器初始化成功: " << m_handType << " " << m_handJoint << std::endl;
            
            // 初始化为张开状态
            openHand();
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "❌ 灵巧手初始化异常: " << e.what() << std::endl;
            return false;
        }
    }
    
private:
    void executeAction(const std::string& actionName) {
        if (!m_pythonInitialized || !m_handInstance) {
            throw std::runtime_error("灵巧手未初始化");
        }
        
        // 设置速度
        PyObject* setSpeedMethod = PyObject_GetAttrString(m_handInstance, "set_speed");
        if (setSpeedMethod) {
            int speedCount = (m_handJoint == "L7") ? 7 : 
                           (m_handJoint == "L10") ? 10 :
                           (m_handJoint == "L20") ? 20 : 25;
            
            PyObject* speedList = PyList_New(speedCount);
            for (int i = 0; i < speedCount; i++) {
                PyList_SetItem(speedList, i, PyLong_FromLong(120));
            }
            PyObject* speedArgs = PyTuple_New(1);
            PyTuple_SetItem(speedArgs, 0, speedList);
            
            PyObject_Call(setSpeedMethod, speedArgs, nullptr);
            
            Py_DECREF(speedArgs);
            Py_DECREF(setSpeedMethod);
        }
        
        // 执行手指移动 - 使用硬编码的位置值
        PyObject* fingerMoveMethod = PyObject_GetAttrString(m_handInstance, "finger_move");
        if (fingerMoveMethod) {
            PyObject* poseList = nullptr;
            
            if (m_handJoint == "L7") {
                poseList = PyList_New(7);
                if (actionName == m_graspAction || actionName == "ZQ") {
                    // 抓取动作 - ZQ
                    double graspPose[7] = {125, 170, 149, 150, 0, 0, 44};
                    for (int i = 0; i < 7; i++) {
                        PyList_SetItem(poseList, i, PyFloat_FromDouble(graspPose[i]));
                    }
                } else if (actionName == m_releaseAction || actionName == "ZK" || actionName == "张开") {
                    // 松开动作 - ZK/张开 - 使用YAML文件中的实际数值
                    double openPose[7] = {255, 179, 255, 255, 255, 255, 83};
                    for (int i = 0; i < 7; i++) {
                        PyList_SetItem(poseList, i, PyFloat_FromDouble(openPose[i]));
                    }
                } else {
                    // 默认张开动作
                    double openPose[7] = {255, 179, 255, 255, 255, 255, 83};
                    for (int i = 0; i < 7; i++) {
                        PyList_SetItem(poseList, i, PyFloat_FromDouble(openPose[i]));
                    }
                }
            } else {
                // 其他手型的默认值
                int jointCount = (m_handJoint == "L10") ? 10 :
                               (m_handJoint == "L20") ? 20 : 25;
                poseList = PyList_New(jointCount);
                
                double defaultValue = (actionName == m_graspAction || actionName == "ZQ") ? 100.0 : 255.0;
                for (int i = 0; i < jointCount; i++) {
                    PyList_SetItem(poseList, i, PyFloat_FromDouble(defaultValue));
                }
            }
            
            PyObject* poseArgs = PyTuple_New(1);
            PyTuple_SetItem(poseArgs, 0, poseList);
            
            PyObject_Call(fingerMoveMethod, poseArgs, nullptr);
            
            Py_DECREF(poseArgs);
            Py_DECREF(fingerMoveMethod);
        }
    }

    void cleanup() {
        if (m_useRos2) {
#ifdef USE_ROS2
            // ROS2模式下清理ROS2资源
            m_ros2Publisher.reset();
            m_ros2Node.reset();
#endif
        } else {
            // Python模式下清理Python资源
            if (m_handInstance) {
                Py_DECREF(m_handInstance);
                m_handInstance = nullptr;
            }
            
            if (m_handModule) {
                Py_DECREF(m_handModule);
                m_handModule = nullptr;
            }
            m_pythonInitialized = false;
        }
    }
    
private:
    void openHandPython() {
        if (!m_pythonInitialized || !m_handInstance) {
            std::cerr << "❌ 灵巧手未初始化" << std::endl;
            return;
        }
        
        try {
            executeAction(m_releaseAction);
        } catch (const std::exception& e) {
            std::cerr << "❌ 张开手掌时发生异常: " << e.what() << std::endl;
        }
    }
    
    void closeHandPython() {
        if (!m_pythonInitialized || !m_handInstance) {
            std::cerr << "❌ 灵巧手未初始化" << std::endl;
            return;
        }
        
        try {
            executeAction(m_graspAction);
        } catch (const std::exception& e) {
            std::cerr << "❌ 握拳时发生异常: " << e.what() << std::endl;
        }
    }
    
#ifdef USE_ROS2
    void publishRos2Command(const std::string& action) {
        if (!m_ros2Node || !m_ros2Publisher) {
            std::cerr << "❌ ROS2节点未初始化" << std::endl;
            return;
        }
        
        try {
            // 创建JSON格式的控制消息
            std::ostringstream json_msg;
            json_msg << "{";
            json_msg << "\"hand_type\":\"" << m_handType << "\",";
            json_msg << "\"hand_joint\":\"" << m_handJoint << "\",";
            json_msg << "\"action\":\"" << action << "\",";
            json_msg << "\"timestamp\":" << std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            json_msg << "}";
            
            auto message = std_msgs::msg::String();
            message.data = json_msg.str();
            
            m_ros2Publisher->publish(message);
            
            std::cout << "📡 ROS2话题发布: " << m_ros2TopicName << " -> " << action << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "❌ ROS2消息发布异常: " << e.what() << std::endl;
        }
    }
#endif

    // ...existing code...
};

// 机械臂控制类
class ArmController {
private:
    int m_socket;
    bool m_connected;
    std::string m_robotIP;
    int m_robotPort;
    
    // 夹爪配置参数
    int m_gripperPickSpeed;
    int m_gripperReleaseSpeed;
    int m_gripperForceThreshold;
    bool m_gripperBlockMode;
    
public:
    ArmController(const std::string& ip = "192.168.10.18", int port = 8080) 
        : m_socket(-1), m_connected(false), m_robotIP(ip), m_robotPort(port),
          m_gripperPickSpeed(500), m_gripperReleaseSpeed(500), 
          m_gripperForceThreshold(200), m_gripperBlockMode(true) {
        #if defined(WIN32)
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        #endif
    }
    
    ~ArmController() {
        disconnect();
        #if defined(WIN32)
        WSACleanup();
        #endif
    }
    
    bool connect() {
        std::cout << "🔄 正在连接机械臂 " << m_robotIP << ":" << m_robotPort << " ..." << std::endl;
        
        // 创建socket
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket < 0) {
            std::cerr << "❌ 创建socket失败" << std::endl;
            return false;
        }
        
        // 设置连接超时
        struct timeval timeout;
        timeout.tv_sec = 3;   // 3秒超时
        timeout.tv_usec = 0;
        
        #if defined(WIN32)
        // Windows socket超时设置
        DWORD timeoutMs = 3000;
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeoutMs, sizeof(timeoutMs));
        #else
        // Linux socket超时设置
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
        #endif
        
        // 设置服务器地址
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(m_robotPort);
        inet_pton(AF_INET, m_robotIP.c_str(), &serverAddr.sin_addr);
        
        // 连接到服务器
        if (::connect(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "❌ 连接机械臂失败: " << m_robotIP << ":" << m_robotPort;
            #if defined(WIN32)
            int errorCode = WSAGetLastError();
            if (errorCode == WSAETIMEDOUT) {
                std::cerr << " (连接超时)" << std::endl;
            } else {
                std::cerr << " (错误代码: " << errorCode << ")" << std::endl;
            }
            closesocket(m_socket);
            #else
            if (errno == ETIMEDOUT || errno == ECONNREFUSED) {
                std::cerr << " (连接超时或拒绝连接)" << std::endl;
            } else {
                std::cerr << " (错误: " << strerror(errno) << ")" << std::endl;
            }
            close(m_socket);
            #endif
            m_socket = -1;
            return false;
        }
        
        m_connected = true;
        std::cout << "✅ 成功连接到机械臂: " << m_robotIP << ":" << m_robotPort << std::endl;
        return true;
    }
    
    void disconnect() {
        if (m_connected && m_socket >= 0) {
            #if defined(WIN32)
            closesocket(m_socket);
            #else
            close(m_socket);
            #endif
            m_connected = false;
            std::cout << "机械臂连接已断开" << std::endl;
        }
    }
    
    bool sendCommand(const std::string& command) {
        if (!m_connected) {
            std::cerr << "❌ TCP错误: 机械臂未连接" << std::endl;
            return false;
        }
        
        // 添加换行符
        std::string cmdWithNewline = command + "\r\n";
        
        // 获取当前时间戳
        auto now = std::chrono::high_resolution_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        std::cout << "\n📤 [TCP发送] " << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
                  << "." << std::setfill('0') << std::setw(3) << ms.count() << std::endl;
        std::cout << "   长度: " << cmdWithNewline.length() << " 字节" << std::endl;
        std::cout << "   内容: " << command << std::endl;
        std::cout << "   原始: " << cmdWithNewline << std::endl;
        
        // 发送命令
        ssize_t sent = send(m_socket, cmdWithNewline.c_str(), cmdWithNewline.length(), 0);
        if (sent < 0) {
            std::cerr << "❌ TCP错误: 发送命令失败, errno=" << errno << std::endl;
            return false;
        }
        
        std::cout << "✅ 发送成功: " << sent << "/" << cmdWithNewline.length() << " 字节" << std::endl;
        return true;
    }
    
    std::string receiveResponse() {
        if (!m_connected) {
            std::cerr << "❌ TCP错误: 连接未建立" << std::endl;
            return "";
        }
        
        char buffer[4096];
        std::string fullResponse = "";
        
        // 设置接收超时
        #if defined(WIN32)
        DWORD timeout = 2000; // 2秒超时
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        #else
        struct timeval tv;
        tv.tv_sec = 2;  // 2秒超时
        tv.tv_usec = 0;
        setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        #endif
        
        // 获取当前时间戳
        auto now = std::chrono::high_resolution_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        std::cout << "\n📥 [TCP接收] " << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
                  << "." << std::setfill('0') << std::setw(3) << ms.count() << std::endl;
        
        // 尝试接收数据
        ssize_t received = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
        if (received > 0) {
            buffer[received] = '\0';
            fullResponse = std::string(buffer);
            std::cout << "✅ 接收成功: " << received << " 字节" << std::endl;
            std::cout << "   内容: " << fullResponse << std::endl;
            
            // 检查是否是完整的JSON
            if (fullResponse.find("{") != std::string::npos && fullResponse.find("}") != std::string::npos) {
                std::cout << "📋 JSON格式: 完整" << std::endl;
            } else {
                std::cout << "⚠️  JSON格式: 可能不完整" << std::endl;
            }
        } else if (received == 0) {
            std::cout << "❌ TCP状态: 连接已关闭" << std::endl;
        } else {
            std::cout << "⏱️  TCP状态: 接收超时或错误, errno=" << errno << std::endl;
        }
        
        return fullResponse;
    }
    
    std::array<int, 6> getCurrentArmPose() {
        // 清空接收缓冲区，避免读取到旧的运动指令确认
        std::cout << "清空接收缓冲区..." << std::endl;
        
        // 先读取并丢弃所有待处理的响应
        for (int i = 0; i < 5; ++i) {
            char buffer[4096];
            #if defined(WIN32)
            DWORD timeout = 100; // 100ms超时
            setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
            #else
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms超时
            setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
            #endif
            
            ssize_t received = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
            if (received <= 0) {
                break; // 没有更多数据了
            }
            std::cout << "丢弃缓冲区数据: " << std::string(buffer, received) << std::endl;
        }
        
        std::cout << "发送获取机械臂状态命令..." << std::endl;
        
        // 发送获取当前状态命令
        if (!sendCommand("{ \"command\": \"get_current_arm_state\" }")) {
            return {0, 0, 0, 0, 0, 0};
        }
        
        // 等待一下确保命令被处理
        #if defined(WIN32)
        Sleep(200);
        #else
        usleep(200000); // 200ms
        #endif
        
        // 接收响应并解析JSON中的pose字段
        std::string response = receiveResponse();
        
        // 如果第一次没有收到正确的arm_state响应，再尝试几次
        int attempts = 0;
        while (attempts < 3 && response.find("\"arm_state\"") == std::string::npos) {
            std::cout << "未收到arm_state响应，重试第" << (attempts + 1) << "次..." << std::endl;
            
            // 再次发送命令
            if (!sendCommand("{ \"command\": \"get_current_arm_state\" }")) {
                return {0, 0, 0, 0, 0, 0};
            }
            
            #if defined(WIN32)
            Sleep(300);
            #else
            usleep(300000); // 300ms
            #endif
            
            response = receiveResponse();
            attempts++;
        }
        
        // 简单的JSON解析来提取arm_state中的pose数组
        std::array<int, 6> pose = {0, 0, 0, 0, 0, 0};
        
        std::cout << "接收到的完整响应: " << response << std::endl;
        
        // 首先查找 "arm_state" 字段
        size_t armStatePos = response.find("\"arm_state\"");
        if (armStatePos != std::string::npos) {
            // 在arm_state内查找 "pose" 字段
            size_t posePos = response.find("\"pose\"", armStatePos);
            if (posePos != std::string::npos) {
                // 查找数组开始位置
                size_t arrayStart = response.find("[", posePos);
                size_t arrayEnd = response.find("]", posePos);
                
                if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
                    std::string poseArray = response.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                    std::cout << "提取到的pose数组字符串: " << poseArray << std::endl;
                    
                    // 解析数组中的6个值
                    std::istringstream iss(poseArray);
                    std::string token;
                    int index = 0;
                    
                    while (std::getline(iss, token, ',') && index < 6) {
                        // 移除空格
                        token.erase(0, token.find_first_not_of(" \t"));
                        token.erase(token.find_last_not_of(" \t") + 1);
                        
                        try {
                            pose[index] = std::stoi(token);
                            std::cout << "解析pose[" << index << "] = " << pose[index] << std::endl;
                            index++;
                        } catch (const std::exception& e) {
                            std::cerr << "解析pose数据错误: " << e.what() << std::endl;
                            break;
                        }
                    }
                    
                    if (index == 6) {
                        std::cout << "成功获取机械臂当前位姿: [";
                        for (int i = 0; i < 6; ++i) {
                            if (i > 0) std::cout << ", ";
                            std::cout << pose[i];
                        }
                        std::cout << "]" << std::endl;
                    } else {
                        std::cerr << "警告: 只解析到 " << index << " 个pose值" << std::endl;
                    }
                } else {
                    std::cerr << "警告: 无法找到pose数组边界" << std::endl;
                }
            } else {
                std::cerr << "警告: 在arm_state中未找到pose字段" << std::endl;
            }
        } else {
            std::cerr << "警告: 响应中未找到arm_state字段" << std::endl;
        }
        
        return pose;
    }
    
    bool moveToTarget(const std::array<int, 6>& targetPose, int velocity = 50) {
        std::ostringstream oss;
        // 使用movep_follow实现笛卡尔空间跟随运动
        oss << "{ \"command\": \"movep_follow\", \"pose\": [";
        for (int i = 0; i < 6; ++i) {
            if (i > 0) oss << ", ";
            oss << targetPose[i];
        }
        oss << "] }";
        
        return sendCommand(oss.str());
    }
    
    bool moveToTargetAsync(const std::array<int, 6>& targetPose, int velocity = 50) {
        if (!m_connected) {
            std::cerr << "❌ TCP错误: 机械臂未连接" << std::endl;
            return false;
        }
        
        // 定期清理接收缓冲区，避免积压（每100次清理一次）
        static int clearCounter = 0;
        static int debugCounter = 0;
        clearCounter++;
        debugCounter++;
        
        if (clearCounter >= 100) {
            clearReceiveBuffer();
            clearCounter = 0;
        }
        
        std::ostringstream oss;
        // 使用movep_follow实现笛卡尔空间跟随运动（最快跟随控制）
        oss << "{ \"command\": \"movep_follow\", \"pose\": [";
        for (int i = 0; i < 6; ++i) {
            if (i > 0) oss << ", ";
            oss << targetPose[i];
        }
        oss << "] }";
        
        // 每50次显示一次详细TCP日志（避免日志过多）
        bool showDebug = (debugCounter % 50 == 0);
        
        if (showDebug) {
            auto now = std::chrono::high_resolution_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
            
            std::cout << "\n🚀 [TCP高频发送] " << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
                      << "." << std::setfill('0') << std::setw(3) << ms.count() << std::endl;
            std::cout << "   频率计数: " << debugCounter << " (每50次显示)" << std::endl;
            std::cout << "   目标位姿: [" << targetPose[0] << ", " << targetPose[1] << ", " 
                      << targetPose[2] << ", " << targetPose[3] << ", " << targetPose[4] << ", " << targetPose[5] << "]" << std::endl;
            std::cout << "   命令内容: " << oss.str() << std::endl;
        }
        
        // 异步发送：只发送不等待响应，确保100Hz频率
        std::string cmdWithNewline = oss.str() + "\r\n";
        ssize_t sent = send(m_socket, cmdWithNewline.c_str(), cmdWithNewline.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
        
        if (showDebug) {
            if (sent > 0) {
                std::cout << "✅ 异步发送成功: " << sent << "/" << cmdWithNewline.length() << " 字节" << std::endl;
            } else {
                std::cout << "❌ 异步发送失败: errno=" << errno << std::endl;
            }
        }
        
        return sent > 0;
    }
    
    // 新增：启用机械臂电源（必须）
    bool enableArmPower() {
        if (!m_connected) {
            return false;
        }
        
        std::string cmd = "{ \"command\": \"set_arm_power\", \"enable\": true }";
        bool result = sendCommand(cmd);
        if (result) {
            usleep(500000); // 等待500ms确保启用完成
        }
        return result;
    }
    
    // 新增：启用角度透传模式（最快响应）
    bool enableAngleTransmission() {
        if (!m_connected) {
            return false;
        }
        
        // 启用角度透传模式，实现最快响应
        std::string cmd = "{ \"command\": \"set_angle_transmission\", \"state\": true }";
        return sendCommand(cmd);
    }
    
    // 新增：启用UDP主动上报（最快状态反馈）
    bool enableUDPBroadcast() {
        if (!m_connected) {
            return false;
        }
        
        // 启用UDP主动上报，获得最快状态反馈
        std::string cmd = "{ \"command\": \"set_realtime_push\", \"cycle\": 5, \"port\": 8089, \"force_coordinate\": 0, \"ip\": \"192.168.10.100\" }";
        return sendCommand(cmd);
    }
    
    // 新增：设置示教参考坐标系
    bool setTeachFrame(int frameType) {
        if (!m_connected) {
            return false;
        }
        
        std::ostringstream oss;
        oss << "{ \"command\": \"set_teach_frame\", \"frame_type\": " << frameType << " }";
        return sendCommand(oss.str());
    }
    
    // 新增：设置工具坐标系（按名称）
    bool setToolCoordinateSystem(const std::string& toolName) {
        if (!m_connected) {
            return false;
        }
        
        std::ostringstream oss;
        oss << "{ \"command\": \"set_tool_coordinate\", \"tool_name\": \"" << toolName << "\" }";
        return sendCommand(oss.str());
    }
    
    // 剪刀控制方法
    bool controlScissors(bool close, int port = 1, int address = 2, int device = 1) {
        if (!m_connected) {
            std::cout << "❌ 剪刀控制失败: 机械臂未连接" << std::endl;
            return false;
        }
        
        std::ostringstream oss;
        int data = close ? 1 : 0;  // 1=闭合剪刀，0=松开剪刀
        
        oss << "{ \"command\": \"write_single_register\", \"port\": " << port 
            << ", \"address\": " << address << ", \"data\": " << data 
            << ", \"device\": " << device << " }";
        
        std::string command = oss.str();
        std::cout << "🔧 发送剪刀控制命令: " << command << std::endl;
        
        return sendCommand(command);
    }
    
    // 剪刀控制方法（自定义数据值）
    bool controlScissorsCustom(int port, int address, int data, int device) {
        if (!m_connected) {
            std::cout << "❌ 剪刀控制失败: 机械臂未连接" << std::endl;
            return false;
        }
        
        std::ostringstream oss;
        
        oss << "{ \"command\": \"write_single_register\", \"port\": " << port 
            << ", \"address\": " << address << ", \"data\": " << data 
            << ", \"device\": " << device << " }";
        
        std::string command = oss.str();
        std::cout << "🔧 发送剪刀控制命令: " << command << std::endl;
        
        return sendCommand(command);
    }
    
    // 新增：直接发送关节角度（最快控制方式）
    bool sendJointAnglesAsync(const std::array<double, 6>& jointAngles) {
        if (!m_connected) {
            return false;
        }
        
        std::ostringstream oss;
        oss << "{ \"command\": \"set_joint_angle_transmission\", \"joint\": [";
        for (int i = 0; i < 6; ++i) {
            if (i > 0) oss << ", ";
            oss << std::fixed << std::setprecision(6) << jointAngles[i];
        }
        oss << "] }";
        
        // 异步发送关节角度
        std::string cmdWithNewline = oss.str() + "\r\n";
        ssize_t sent = send(m_socket, cmdWithNewline.c_str(), cmdWithNewline.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
        
        return sent > 0;
    }
    
    void clearReceiveBuffer() {
        if (!m_connected) return;
        
        // 非阻塞读取并丢弃所有待处理数据
        char buffer[4096];
        int totalCleared = 0;
        int packets = 0;
        
        std::cout << "\n🧹 [TCP缓冲区清理] 开始清理..." << std::endl;
        
        while (true) {
            ssize_t received = recv(m_socket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
            if (received <= 0) {
                break; // 没有更多数据
            }
            
            buffer[received] = '\0';
            totalCleared += received;
            packets++;
            
            // 显示清理的数据（只显示前100个字符避免过长）
            std::string data(buffer);
            if (data.length() > 100) {
                data = data.substr(0, 100) + "...";
            }
            std::cout << "   清理包" << packets << ": " << received << "字节 - " << data << std::endl;
        }
        
        if (totalCleared > 0) {
            std::cout << "✅ 清理完成: 共" << packets << "个包, " << totalCleared << "字节" << std::endl;
        } else {
            std::cout << "✅ 缓冲区已清洁" << std::endl;
        }
    }
    
    // 新增：夹抓控制方法 - 使用力矩抓取和释放
    bool controlGripper(bool open) {
        if (!m_connected) {
            std::cerr << "❌ 夹抓控制错误: 机械臂未连接" << std::endl;
            return false;
        }
        
        std::ostringstream oss;
        if (open) {
            // 打开夹抓 - 使用释放命令
            oss << "{ \"command\": \"set_gripper_release\", \"speed\": " << m_gripperReleaseSpeed 
                << ", \"block\": " << (m_gripperBlockMode ? "true" : "false") << " }";
            std::cout << "\n🤏 [夹抓控制] 释放夹抓 (速度: " << m_gripperReleaseSpeed << ")" << std::endl;
        } else {
            // 关闭夹抓 - 使用力矩抓取
            oss << "{ \"command\": \"set_gripper_pick\", \"speed\": " << m_gripperPickSpeed 
                << ", \"force\": " << m_gripperForceThreshold 
                << ", \"block\": " << (m_gripperBlockMode ? "true" : "false") << " }";
            std::cout << "\n🤏 [夹抓控制] 力矩抓取 (速度: " << m_gripperPickSpeed 
                      << ", 力矩: " << m_gripperForceThreshold << ")" << std::endl;
        }
        
        return sendCommand(oss.str());
    }
    
    // 新增：设置夹爪配置参数
    void setGripperConfig(int pickSpeed, int releaseSpeed, int forceThreshold, bool blockMode) {
        m_gripperPickSpeed = pickSpeed;
        m_gripperReleaseSpeed = releaseSpeed;
        m_gripperForceThreshold = forceThreshold;
        m_gripperBlockMode = blockMode;
        
        std::cout << "夹爪配置已更新: 抓取速度=" << pickSpeed 
                  << ", 释放速度=" << releaseSpeed 
                  << ", 力控阈值=" << forceThreshold 
                  << ", 阻塞模式=" << (blockMode ? "是" : "否") << std::endl;
    }
    
    bool isConnected() const { return m_connected; }
};

// 触觉设备机械臂控制器
class TouchArmController {
private:
    bool m_dragging;
    std::array<double, 3> m_touchAnchor;      // 触觉设备锚点
    std::array<double, 16> m_touchAnchorTransform;  // 触觉设备锚点姿态
    std::array<int, 6> m_armAnchor;           // 机械臂锚点位姿
    double m_positionScale;    // 位置映射系数
    double m_rotationScale;    // 姿态映射系数
    double m_springStiffness;  // 弹簧刚度系数
    int m_debugFrequency;      // 调试信息显示频率
    int m_controlFrequency;    // 控制命令发送频率
    
    // 坐标映射配置变量
    int m_touchPosToArmX;      // 触觉设备哪个轴(0=X,1=Y,2=Z)映射到机械臂X轴
    int m_touchPosToArmY;      // 触觉设备哪个轴(0=X,1=Y,2=Z)映射到机械臂Y轴
    int m_touchPosToArmZ;      // 触觉设备哪个轴(0=X,1=Y,2=Z)映射到机械臂Z轴
    int m_touchRotToArmRX;     // 触觉设备哪个旋转轴(0=RX,1=RY,2=RZ)映射到机械臂RX轴
    int m_touchRotToArmRY;     // 触觉设备哪个旋转轴(0=RX,1=RY,2=RZ)映射到机械臂RY轴
    int m_touchRotToArmRZ;     // 触觉设备哪个旋转轴(0=RX,1=RY,2=RZ)映射到机械臂RZ轴
    
    // 机械臂轴符号调整 (1或-1)
    int m_armXSign;            // 机械臂X轴符号 (1或-1)
    int m_armYSign;            // 机械臂Y轴符号 (1或-1)
    int m_armZSign;            // 机械臂Z轴符号 (1或-1)
    int m_armRXSign;           // 机械臂RX轴符号 (1或-1)
    int m_armRYSign;           // 机械臂RY轴符号 (1或-1)
    int m_armRZSign;           // 机械臂RZ轴符号 (1或-1)
    
    ArmController& m_armController;
    ConfigLoader* m_config;    // 配置文件加载器
    std::string m_deviceName;  // 设备名称
    
    // 灵巧手相关成员
    bool m_useDexterousHand;           // 是否使用灵巧手
    DexterousHandController* m_handController;  // 灵巧手控制器
    
    // 末端控制器类型和剪刀控制相关成员
    std::string m_endEffectorType;     // 末端控制器类型 (dexterous_hand, gripper, scissors)
    int m_scissorsModbusPort;          // 剪刀Modbus端口
    int m_scissorsModbusAddress;       // 剪刀寄存器地址
    int m_scissorsModbusDevice;        // 剪刀设备号
    int m_scissorsOpenData;            // 松开剪刀的数据值
    int m_scissorsCloseData;           // 闭合剪刀的数据值
    bool m_scissorsState;              // 剪刀状态 (true=闭合, false=松开)
    
public:
    TouchArmController(ArmController& armController, ConfigLoader* config = nullptr, const std::string& deviceName = "") 
        : m_dragging(false), m_armController(armController), m_config(config), m_deviceName(deviceName),
          m_touchAnchor({0.0, 0.0, 0.0}),
          m_touchAnchorTransform({1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0}),
          m_armAnchor({0, 0, 0, 0, 0, 0}),
          m_touchPosToArmX(2), m_touchPosToArmY(0), m_touchPosToArmZ(1),
          m_touchRotToArmRX(2), m_touchRotToArmRY(0), m_touchRotToArmRZ(1),
          m_armXSign(-1), m_armYSign(-1), m_armZSign(1), 
          m_armRXSign(-1), m_armRYSign(-1), m_armRZSign(1),
          m_useDexterousHand(false), m_handController(nullptr),
          m_endEffectorType("gripper"), m_scissorsModbusPort(1), m_scissorsModbusAddress(2),
          m_scissorsModbusDevice(1), m_scissorsOpenData(0), m_scissorsCloseData(1), m_scissorsState(false) {
        
        // 从配置文件加载参数，如果没有配置文件则使用默认值
        if (m_config) {
            std::string prefix = m_deviceName.empty() ? "control" : m_deviceName;
            m_positionScale = m_config->getDouble(prefix + ".position_scale", 1000.0);
            m_rotationScale = m_config->getDouble(prefix + ".rotation_scale", 1.0);
            m_springStiffness = m_config->getDouble(prefix + ".spring_stiffness", 0.2);
            m_debugFrequency = m_config->getInt("system.debug_frequency", 50);
            m_controlFrequency = m_config->getInt("system.control_frequency", 10);
            
            // 加载坐标映射配置
            std::string mappingPrefix = m_deviceName.empty() ? "mapping" : (m_deviceName + "_mapping");
            m_touchPosToArmX = m_config->getInt(mappingPrefix + ".touch_pos_to_arm_x", 2);  // 默认：Z→X
            m_touchPosToArmY = m_config->getInt(mappingPrefix + ".touch_pos_to_arm_y", 0);  // 默认：X→Y
            m_touchPosToArmZ = m_config->getInt(mappingPrefix + ".touch_pos_to_arm_z", 1);  // 默认：Y→Z
            m_touchRotToArmRX = m_config->getInt(mappingPrefix + ".touch_rot_to_arm_rx", 2); // 默认：RZ→RX
            m_touchRotToArmRY = m_config->getInt(mappingPrefix + ".touch_rot_to_arm_ry", 0); // 默认：RX→RY
            m_touchRotToArmRZ = m_config->getInt(mappingPrefix + ".touch_rot_to_arm_rz", 1); // 默认：RY→RZ
            
            // 加载机械臂轴符号调整
            m_armXSign = m_config->getInt(mappingPrefix + ".arm_x_sign", 1);
            m_armYSign = m_config->getInt(mappingPrefix + ".arm_y_sign", 1);
            m_armZSign = m_config->getInt(mappingPrefix + ".arm_z_sign", 1);
            m_armRXSign = m_config->getInt(mappingPrefix + ".arm_rx_sign", -1);
            m_armRYSign = m_config->getInt(mappingPrefix + ".arm_ry_sign", -1);
            m_armRZSign = m_config->getInt(mappingPrefix + ".arm_rz_sign", 1);
            
            // 加载末端控制器配置
            m_endEffectorType = m_config->getString(mappingPrefix + ".end_effector_type", "gripper");
            
            // 加载剪刀控制配置
            m_scissorsModbusPort = m_config->getInt(mappingPrefix + ".scissors_modbus_port", 1);
            m_scissorsModbusAddress = m_config->getInt(mappingPrefix + ".scissors_modbus_address", 2);
            m_scissorsModbusDevice = m_config->getInt(mappingPrefix + ".scissors_modbus_device", 1);
            m_scissorsOpenData = m_config->getInt(mappingPrefix + ".scissors_open_data", 0);
            m_scissorsCloseData = m_config->getInt(mappingPrefix + ".scissors_close_data", 1);
            
            std::cout << "从配置文件加载" << m_deviceName << "控制参数和坐标映射配置" << std::endl;
        } else {
            // 使用默认值
            m_positionScale = 1000.0;
            m_rotationScale = 1.0;
            m_springStiffness = 0.2;
            m_debugFrequency = 50;
            m_controlFrequency = 10;
            
            // 默认映射配置（保持与原代码相同的行为）
            m_touchPosToArmX = 2;   // 触觉设备Z轴 → 机械臂X轴
            m_touchPosToArmY = 0;   // 触觉设备X轴 → 机械臂Y轴
            m_touchPosToArmZ = 1;   // 触觉设备Y轴 → 机械臂Z轴
            m_touchRotToArmRX = 2;  // 触觉设备RZ轴 → 机械臂RX轴
            m_touchRotToArmRY = 0;  // 触觉设备RX轴 → 机械臂RY轴
            m_touchRotToArmRZ = 1;  // 触觉设备RY轴 → 机械臂RZ轴
            
            // 默认符号调整
            m_armXSign = -1;    // 机械臂X轴取反
            m_armYSign = -1;    // 机械臂Y轴取反
            m_armZSign = 1;     // 机械臂Z轴正向
            m_armRXSign = -1;   // 机械臂RX轴取反
            m_armRYSign = -1;   // 机械臂RY轴取反
            m_armRZSign = 1;    // 机械臂RZ轴正向
            
            std::cout << "使用默认控制参数和坐标映射配置 (" << m_deviceName << ")" << std::endl;
        }
        
        // 初始化末端控制器配置
        initializeEndEffector();
        
        // 启用机械臂控制模式
        if (m_armController.isConnected()) {
            std::cout << "启用机械臂控制模式..." << std::endl;
            
            // 0. 启用机械臂电源（必须第一步）
            m_armController.enableArmPower();
            
            // 1. 启用角度透传模式（最快响应）
            m_armController.enableAngleTransmission();
            
            // 2. 启用UDP主动上报（最快状态反馈）
            m_armController.enableUDPBroadcast();
            
            // 3. 设置示教参考坐标系（从配置文件读取）
            int frameType = 1;  // 默认为工具坐标系
            if (m_config) {
                frameType = m_config->getInt("system.teach_frame_type", 1);
            }
            std::cout << "设置示教参考坐标系为: " << (frameType == 0 ? "基坐标系" : "工具坐标系") << "..." << std::endl;
            m_armController.setTeachFrame(frameType);
            
            // 4. 设置工具坐标系（根据实曼协议）
            std::string toolName = "Arm_Tip";  // 默认值
            if (m_config) {
                toolName = m_config->getString("system.tool_coordinate_name", "Arm_Tip");
            }
            std::cout << "设置工具坐标系为: " << toolName << "..." << std::endl;
            m_armController.setToolCoordinateSystem(toolName);
            
            // 5. 配置夹爪参数（从配置文件读取）
            int pickSpeed = 500, releaseSpeed = 500, forceThreshold = 200;
            bool blockMode = true;
            if (m_config) {
                pickSpeed = m_config->getInt("gripper.pick_speed", 500);
                releaseSpeed = m_config->getInt("gripper.release_speed", 500);
                forceThreshold = m_config->getInt("gripper.force_threshold", 200);
                blockMode = m_config->getBool("gripper.block_mode", true);
            }
            std::cout << "配置夹爪参数..." << std::endl;
            m_armController.setGripperConfig(pickSpeed, releaseSpeed, forceThreshold, blockMode);
            
            std::cout << "机械臂控制模式已启用" << std::endl;
            std::cout << "参考坐标系: " << (frameType == 0 ? "基坐标系" : "工具坐标系") << std::endl;
            std::cout << "工具坐标系: " << toolName << std::endl;
        }
    }
    
    // 添加析构函数
    ~TouchArmController() {
        if (m_handController) {
            delete m_handController;
            m_handController = nullptr;
        }
    }
    
    // 初始化末端控制器
    void initializeEndEffector() {
        if (m_config) {
            std::string prefix = m_deviceName + "_mapping";
            
            std::cout << "[" << m_deviceName << "] 末端控制器类型: " << m_endEffectorType << std::endl;
            
            if (m_endEffectorType == "dexterous_hand") {
                // 初始化灵巧手
                m_useDexterousHand = true;
                
                std::string handType = m_config->getString(prefix + ".hand_type", "left");
                std::string handJoint = m_config->getString(prefix + ".hand_joint", "L10");
                std::string canInterface = m_config->getString(prefix + ".can_interface", "can0");
                std::string graspAction = m_config->getString(prefix + ".grasp_action", "ZQ");
                std::string releaseAction = m_config->getString(prefix + ".release_action", "张开");
                bool useRos2 = m_config->getBool(prefix + ".use_ros2", false);
                std::string ros2TopicName = m_config->getString(prefix + ".ros2_topic_name", "/dexterous_hand/command");
                
                // 调试输出
                std::cout << "🔍 调试信息: prefix=" << prefix << ", use_ros2键=" << (prefix + ".use_ros2") << ", 读取值=" << (useRos2 ? "true" : "false") << std::endl;
                
                std::cout << "[" << m_deviceName << "] 正在初始化灵巧手..." << std::endl;
                std::cout << "   手型: " << handType << std::endl;
                std::cout << "   关节: " << handJoint << std::endl;
                if (useRos2) {
                    std::cout << "   控制方式: ROS2话题" << std::endl;
                    std::cout << "   话题名称: " << ros2TopicName << std::endl;
                } else {
                    std::cout << "   控制方式: CAN直接控制" << std::endl;
                    std::cout << "   CAN接口: " << canInterface << std::endl;
                }
                std::cout << "   抓取动作: " << graspAction << std::endl;
                std::cout << "   松开动作: " << releaseAction << std::endl;
                
                m_handController = new DexterousHandController(handType, handJoint, canInterface, 
                                                             graspAction, releaseAction, useRos2, ros2TopicName);
                
                if (m_handController->initialize()) {
                    std::cout << "✅ [" << m_deviceName << "] 灵巧手初始化成功" << std::endl;
                } else {
                    std::cout << "❌ [" << m_deviceName << "] 灵巧手初始化失败，将使用夹爪模式" << std::endl;
                    delete m_handController;
                    m_handController = nullptr;
                    m_useDexterousHand = false;
                    m_endEffectorType = "gripper";
                }
            } else if (m_endEffectorType == "scissors") {
                // 初始化剪刀控制
                std::cout << "[" << m_deviceName << "] 正在初始化剪刀控制..." << std::endl;
                std::cout << "   Modbus端口: " << m_scissorsModbusPort << std::endl;
                std::cout << "   寄存器地址: " << m_scissorsModbusAddress << std::endl;
                std::cout << "   设备号: " << m_scissorsModbusDevice << std::endl;
                std::cout << "   松开数据值: " << m_scissorsOpenData << std::endl;
                std::cout << "   闭合数据值: " << m_scissorsCloseData << std::endl;
                std::cout << "✅ [" << m_deviceName << "] 剪刀控制配置完成" << std::endl;
                
                m_useDexterousHand = false;
                m_handController = nullptr;
            } else {
                // 使用夹爪模式
                std::cout << "[" << m_deviceName << "] 配置为使用夹爪模式" << std::endl;
                m_useDexterousHand = false;
                m_handController = nullptr;
            }
        } else {
            std::cout << "[" << m_deviceName << "] 无配置文件，默认使用夹爪模式" << std::endl;
            m_useDexterousHand = false;
            m_endEffectorType = "gripper";
        }
    }
    
    void setPositionScale(double scale) {
        m_positionScale = scale;
        std::cout << "[" << m_deviceName << "] 位置映射系数设置为: " << scale << std::endl;
        // 保存到配置文件
        if (m_config) {
            std::string prefix = m_deviceName.empty() ? "control" : m_deviceName;
            m_config->setDouble(prefix + ".position_scale", scale);
        }
    }
    
    void setRotationScale(double scale) {
        m_rotationScale = scale;
        std::cout << "[" << m_deviceName << "] 姿态映射系数设置为: " << scale << std::endl;
        // 保存到配置文件
        if (m_config) {
            std::string prefix = m_deviceName.empty() ? "control" : m_deviceName;
            m_config->setDouble(prefix + ".rotation_scale", scale);
        }
    }
    
    double getPositionScale() const { return m_positionScale; }
    double getRotationScale() const { return m_rotationScale; }
    
    void setSpringStiffness(double stiffness) {
        m_springStiffness = stiffness;
        std::cout << "[" << m_deviceName << "] 弹簧刚度设置为: " << stiffness << std::endl;
        // 保存到配置文件
        if (m_config) {
            std::string prefix = m_deviceName.empty() ? "control" : m_deviceName;
            m_config->setDouble(prefix + ".spring_stiffness", stiffness);
        }
    }
    
    double getSpringStiffness() const { return m_springStiffness; }
    
    // 保存当前配置到文件
    void saveConfig() {
        if (m_config) {
            m_config->saveConfig();
        }
    }
    
    void onButtonDown(const std::array<double, 3>& touchPos, 
                     const std::array<double, 16>& touchTransform) {
        if (!m_dragging) {
            if (m_armController.isConnected()) {
                std::cout << "\n=== 开始新的拖动控制 ===" << std::endl;
            
            // 记录触觉设备锚点
            m_touchAnchor = touchPos;
            m_touchAnchorTransform = touchTransform;
            
            std::cout << "步骤1: 记录触觉设备锚点位置和姿态" << std::endl;
            // 显示配置映射后的坐标信息
            std::cout << "  触觉设备锚点: [" 
                      << std::fixed << std::setprecision(2)
                      << touchPos[0] << ", " << touchPos[1] << ", " << touchPos[2] << "] (原始X,Y,Z)" << std::endl;
            std::cout << "  坐标映射: 触觉设备[" << m_touchPosToArmX << "," << m_touchPosToArmY << "," << m_touchPosToArmZ << "]→机械臂[X,Y,Z]" << std::endl;
            
            std::cout << "步骤2: 获取机械臂当前真实位姿作为锚点..." << std::endl;
            
            // 每次按下都重新获取机械臂当前位姿作为锚点
            std::array<int, 6> newArmPose = m_armController.getCurrentArmPose();
            
            // 检查是否成功获取到有效的位姿数据
            bool validPose = false;
            for (int i = 0; i < 6; ++i) {
                if (newArmPose[i] != 0) {
                    validPose = true;
                    break;
                }
            }
            
            if (validPose || (newArmPose[0] == 0 && newArmPose[1] == 0 && newArmPose[2] == 0)) {
                m_armAnchor = newArmPose;
                m_dragging = true;
                
                std::cout << "步骤3: 成功设置机械臂锚点位姿: [";
                for (int i = 0; i < 6; ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << m_armAnchor[i];
                }
                std::cout << "]" << std::endl;
                std::cout << "  位置 (微米): X=" << m_armAnchor[0] << ", Y=" << m_armAnchor[1] << ", Z=" << m_armAnchor[2] << std::endl;
                std::cout << "  姿态 (毫弧度): RX=" << m_armAnchor[3] << ", RY=" << m_armAnchor[4] << ", RZ=" << m_armAnchor[5] << std::endl;
                std::cout << "=== 拖动控制已激活 ===\n" << std::endl;
            } else {
                std::cout << "警告: 获取机械臂位姿失败，无法开始拖动控制" << std::endl;
                std::cout << "请检查机械臂连接和状态\n" << std::endl;
            }
            } else {
                // 机械臂未连接，仍然记录触觉设备锚点以提供触觉反馈
                m_touchAnchor = touchPos;
                m_touchAnchorTransform = touchTransform;
                m_dragging = true;
                
                std::cout << "\n=== 触觉反馈模式激活 ===" << std::endl;
                std::cout << "[" << m_deviceName << "] 机械臂未连接，启用纯触觉反馈模式" << std::endl;
                std::cout << "  触觉设备锚点: [" 
                          << std::fixed << std::setprecision(2)
                          << touchPos[0] << ", " << touchPos[1] << ", " << touchPos[2] << "] (原始X,Y,Z)" << std::endl;
                std::cout << "  坐标映射: 触觉设备[" << m_touchPosToArmX << "," << m_touchPosToArmY << "," << m_touchPosToArmZ << "]→机械臂[X,Y,Z]" << std::endl;
                std::cout << "🎮 您可以移动设备体验触觉反馈，但机械臂不会移动" << std::endl;
                std::cout << "=== 触觉反馈模式已激活 ===\n" << std::endl;
            }
        } else if (m_dragging) {
            std::cout << "拖动控制已在进行中，请先松开按钮" << std::endl;
        }
    }
    
    void onButtonUp() {
        if (m_dragging) {
            m_dragging = false;
            if (m_armController.isConnected()) {
                std::cout << "\n=== 结束拖动控制 ===" << std::endl;
                std::cout << "触觉设备按钮已松开，机械臂控制停止" << std::endl;
                std::cout << "机械臂将保持在当前位置" << std::endl;
                std::cout << "下次按下按钮时将重新获取机械臂位姿作为新锚点\n" << std::endl;
            } else {
                std::cout << "\n=== 结束触觉反馈模式 ===" << std::endl;
                std::cout << "[" << m_deviceName << "] 触觉设备按钮已松开，触觉反馈停止" << std::endl;
                std::cout << "下次按下按钮时将重新启动触觉反馈模式\n" << std::endl;
            }
        }
    }
    
    void update(const std::array<double, 3>& touchPos, 
               const std::array<double, 16>& touchTransform) {
        if (!m_dragging) {
            return;
        }
        
        // 使用可配置的坐标映射
        std::array<double, 3> relativeTouchPos = {
            m_armXSign * (touchPos[m_touchPosToArmX] - m_touchAnchor[m_touchPosToArmX]) * m_positionScale,  // 机械臂X
            m_armYSign * (touchPos[m_touchPosToArmY] - m_touchAnchor[m_touchPosToArmY]) * m_positionScale,  // 机械臂Y
            m_armZSign * (touchPos[m_touchPosToArmZ] - m_touchAnchor[m_touchPosToArmZ]) * m_positionScale   // 机械臂Z
        };
        
        // 使用可配置的姿态映射
        std::array<double, 3> currentEuler = extractEulerAngles(touchTransform);
        std::array<double, 3> anchorEuler = extractEulerAngles(m_touchAnchorTransform);
        
        std::array<double, 3> relativeRotation = {
            m_armRXSign * (currentEuler[m_touchRotToArmRX] - anchorEuler[m_touchRotToArmRX]) * m_rotationScale * M_PI / 180.0 * 1000, // 机械臂RX
            m_armRYSign * (currentEuler[m_touchRotToArmRY] - anchorEuler[m_touchRotToArmRY]) * m_rotationScale * M_PI / 180.0 * 1000, // 机械臂RY
            m_armRZSign * (currentEuler[m_touchRotToArmRZ] - anchorEuler[m_touchRotToArmRZ]) * m_rotationScale * M_PI / 180.0 * 1000  // 机械臂RZ
        };
        
        // 计算目标机械臂位姿 (单位：微米和毫弧度)
        std::array<int, 6> targetPose = {
            m_armAnchor[0] + static_cast<int>(relativeTouchPos[0]),  // X
            m_armAnchor[1] + static_cast<int>(relativeTouchPos[1]),  // Y  
            m_armAnchor[2] + static_cast<int>(relativeTouchPos[2]),  // Z
            m_armAnchor[3] + static_cast<int>(relativeRotation[0]),  // RX
            m_armAnchor[4] + static_cast<int>(relativeRotation[1]),  // RY
            m_armAnchor[5] + static_cast<int>(relativeRotation[2])   // RZ
        };
        
        // 控制机械臂移动到目标位姿 - 100Hz控制频率
        static auto lastSendTime = std::chrono::high_resolution_clock::now();
        static int debugCounter = 0;
        
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto timeSinceLastSend = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastSendTime).count();
        
        // 使用配置文件中的控制频率
        if (timeSinceLastSend >= m_controlFrequency) {
            // 使用配置文件中的调试频率
            debugCounter++;
            if (debugCounter >= m_debugFrequency) {
                // 更详细的调试信息，包含设备标识
                std::cout << "=== [" << m_deviceName << "] 位置控制调试 ===" << std::endl;
                std::cout << "[" << m_deviceName << "] 触觉设备变化: [" << std::fixed << std::setprecision(3)
                          << (touchPos[m_touchPosToArmX] - m_touchAnchor[m_touchPosToArmX]) << ", " 
                          << (touchPos[m_touchPosToArmY] - m_touchAnchor[m_touchPosToArmY]) << ", " 
                          << (touchPos[m_touchPosToArmZ] - m_touchAnchor[m_touchPosToArmZ]) << "] mm (X,Y,Z)" << std::endl;
                std::cout << "[" << m_deviceName << "] 坐标轴映射: 触觉设备[" << m_touchPosToArmX << "," << m_touchPosToArmY << "," << m_touchPosToArmZ << "]→机械臂[X,Y,Z]" << std::endl;
                std::cout << "[" << m_deviceName << "] 姿态轴映射: 触觉设备[" << m_touchRotToArmRX << "," << m_touchRotToArmRY << "," << m_touchRotToArmRZ << "]→机械臂[RX,RY,RZ]" << std::endl;
                std::cout << "[" << m_deviceName << "] 符号调整: [" << m_armXSign << "," << m_armYSign << "," << m_armZSign << "," << m_armRXSign << "," << m_armRYSign << "," << m_armRZSign << "]" << std::endl;
                std::cout << "[" << m_deviceName << "] 触觉姿态变化: [" << std::fixed << std::setprecision(2)
                          << (currentEuler[m_touchRotToArmRX] - anchorEuler[m_touchRotToArmRX]) << ", " 
                          << (currentEuler[m_touchRotToArmRY] - anchorEuler[m_touchRotToArmRY]) << ", " 
                          << (currentEuler[m_touchRotToArmRZ] - anchorEuler[m_touchRotToArmRZ]) << "] deg (RX,RY,RZ)" << std::endl;
                std::cout << "[" << m_deviceName << "] 映射后增量: [" << std::fixed << std::setprecision(1)
                          << relativeTouchPos[0] << ", " << relativeTouchPos[1] << ", " << relativeTouchPos[2] << "] μm" << std::endl;
                std::cout << "[" << m_deviceName << "] 映射后姿态: [" << std::fixed << std::setprecision(1)
                          << relativeRotation[0] << ", " << relativeRotation[1] << ", " << relativeRotation[2] << "] mrad" << std::endl;
                std::cout << "[" << m_deviceName << "] 机械臂目标位姿: [";
                for (int i = 0; i < 6; ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << targetPose[i];
                }
                std::cout << "] (前3个为μm, 后3个为mrad)" << std::endl;
                std::cout << "[" << m_deviceName << "] 位置映射系数: " << m_positionScale << ", 姿态映射系数: " << m_rotationScale << std::endl;
                std::cout << "============================================" << std::endl;
                debugCounter = 0;
            }
            
            // 只在机械臂连接时才发送控制命令
            if (m_armController.isConnected()) {
                // 使用最快的控制方式：笛卡尔空间跟随运动
                m_armController.moveToTargetAsync(targetPose, 90);
            }
            lastSendTime = currentTime;
        }
    }
    
    bool isDragging() const { return m_dragging; }
    
    std::array<double, 3> getTouchAnchor() const { return m_touchAnchor; }
    
    void queryCurrentArmState() {
        if (m_armController.isConnected()) {
            std::cout << "\n=== 查询机械臂当前状态 ===" << std::endl;
            std::array<int, 6> currentPose = m_armController.getCurrentArmPose();
            
            std::cout << "当前机械臂位姿: [";
            for (int i = 0; i < 6; ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << currentPose[i];
            }
            std::cout << "]" << std::endl;
            std::cout << "  位置 (微米): X=" << currentPose[0] << ", Y=" << currentPose[1] << ", Z=" << currentPose[2] << std::endl;
            std::cout << "  姿态 (毫弧度): RX=" << currentPose[3] << ", RY=" << currentPose[4] << ", RZ=" << currentPose[5] << std::endl;
            
            if (m_dragging) {
                std::cout << "拖动状态: 活动中 (基于锚点位姿: [";
                for (int i = 0; i < 6; ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << m_armAnchor[i];
                }
                std::cout << "])" << std::endl;
            } else {
                std::cout << "拖动状态: 非活动 (下次按下按钮将设置新锚点)" << std::endl;
            }
            std::cout << "==============================\n" << std::endl;
        } else {
            std::cout << "\n=== 查询机械臂当前状态 ===" << std::endl;
            std::cout << "[" << m_deviceName << "] 机械臂连接状态: 未连接" << std::endl;
            std::cout << "🎮 Touch设备状态: 正常工作，可提供触觉反馈" << std::endl;
            std::cout << "📡 机械臂控制: 暂不可用" << std::endl;
            std::cout << "🔧 请检查网络连接和机械臂状态" << std::endl;
            std::cout << "==============================\n" << std::endl;
        }
    }
    
    // 新增：夹抓控制方法（根据末端控制器类型切换模式）
    void onGripperButtonPressed() {
        if (m_endEffectorType == "dexterous_hand" && m_useDexterousHand && 
            m_handController && m_handController->isInitialized()) {
            // 使用灵巧手控制
            std::cout << "\n=== 灵巧手控制 ===" << std::endl;
            std::cout << "[" << m_deviceName << "] 按钮2按下 - 切换灵巧手状态" << std::endl;
            
            if (m_handController->isHandOpen()) {
                std::cout << "灵巧手状态: 张开 → 握拳" << std::endl;
                m_handController->closeHand();
            } else {
                std::cout << "灵巧手状态: 握拳 → 张开" << std::endl;
                m_handController->openHand();
            }
            
            std::cout << "当前灵巧手状态: " << (m_handController->isHandOpen() ? "张开" : "握拳") << std::endl;
            std::cout << "==================\n" << std::endl;
            
        } else if (m_endEffectorType == "scissors" && m_armController.isConnected()) {
            // 使用剪刀控制
            std::cout << "\n=== 剪刀控制 ===" << std::endl;
            std::cout << "[" << m_deviceName << "] 按钮2按下 - 切换剪刀状态" << std::endl;
            
            if (m_scissorsState) {
                // 当前是闭合状态，切换为松开
                std::cout << "剪刀状态: 闭合 → 松开" << std::endl;
                m_armController.controlScissorsCustom(m_scissorsModbusPort, m_scissorsModbusAddress, 
                                                     m_scissorsOpenData, m_scissorsModbusDevice);
                m_scissorsState = false;
            } else {
                // 当前是松开状态，切换为闭合
                std::cout << "剪刀状态: 松开 → 闭合" << std::endl;
                m_armController.controlScissorsCustom(m_scissorsModbusPort, m_scissorsModbusAddress, 
                                                     m_scissorsCloseData, m_scissorsModbusDevice);
                m_scissorsState = true;
            }
            
            std::cout << "当前剪刀状态: " << (m_scissorsState ? "闭合" : "松开") << std::endl;
            std::cout << "================\n" << std::endl;
            
        } else if (m_endEffectorType == "gripper" && m_armController.isConnected()) {
            // 使用夹爪控制
            static bool gripperClosed = false;  // 记录当前夹抓状态，默认为打开
            
            std::cout << "\n=== 夹爪控制 ===" << std::endl;
            if (gripperClosed) {
                // 当前是关闭状态，切换为打开
                std::cout << "[" << m_deviceName << "] 按钮2按下 - 打开夹爪" << std::endl;
                m_armController.controlGripper(true);   // 打开夹爪
                gripperClosed = false;
            } else {
                // 当前是打开状态，切换为关闭
                std::cout << "[" << m_deviceName << "] 按钮2按下 - 关闭夹爪" << std::endl;
                m_armController.controlGripper(false);  // 关闭夹爪
                gripperClosed = true;
            }
            std::cout << "当前夹爪状态: " << (gripperClosed ? "关闭" : "打开") << std::endl;
            std::cout << "================\n" << std::endl;
        } else {
            std::cout << "\n=== 末端控制 ===" << std::endl;
            std::cout << "[" << m_deviceName << "] 按钮2按下 - 无可用的末端控制设备" << std::endl;
            std::cout << "末端控制器类型: " << m_endEffectorType << std::endl;
            
            if (m_endEffectorType == "dexterous_hand") {
                std::cout << "❌ 灵巧手未正确初始化" << std::endl;
            } else if (m_endEffectorType == "scissors" || m_endEffectorType == "gripper") {
                std::cout << "❌ 机械臂未连接，无法控制末端设备" << std::endl;
            }
            
            std::cout << "🎮 Touch设备工作正常，但末端控制不可用" << std::endl;
            std::cout << "🔧 请检查设备连接和配置后重试" << std::endl;
            std::cout << "================\n" << std::endl;
        }
    }

private:
    std::array<double, 3> extractEulerAngles(const std::array<double, 16>& transform) const {
        // 从4x4变换矩阵提取欧拉角（XYZ顺序）
        double r11 = transform[0];
        double r12 = transform[4];
        double r13 = transform[8];
        double r21 = transform[1];
        double r22 = transform[5];
        double r23 = transform[9];
        double r31 = transform[2];
        double r32 = transform[6];
        double r33 = transform[10];
        
        double roll, pitch, yaw;
        
        pitch = std::asin(-r31);
        
        if (std::cos(pitch) > 0.0001) {
            roll = std::atan2(r32, r33);
            yaw = std::atan2(r21, r11);
        } else {
            roll = std::atan2(-r23, r22);
            yaw = 0.0;
        }
        
        // 转换为度
        roll = roll * 180.0 / M_PI;
        pitch = pitch * 180.0 / M_PI;
        yaw = yaw * 180.0 / M_PI;
        
        return {roll, pitch, yaw};
    }
};

// 全局变量 - 双设备支持
ConfigLoader* g_config = nullptr;  // 改为指针，支持动态配置文件
ArmController* g_armController1 = nullptr;    // 机械臂1控制器
ArmController* g_armController2 = nullptr;    // 机械臂2控制器
TouchArmController* g_touchArmController1 = nullptr;  // 触觉设备1控制器
TouchArmController* g_touchArmController2 = nullptr;  // 触觉设备2控制器
bool g_applicationRunning = true;
int g_selectedDevice = 1;  // 当前选择的设备（1或2），用于调整参数

// 双设备句柄
static HHD g_hHD1 = HD_INVALID_HANDLE;  // 设备1
static HHD g_hHD2 = HD_INVALID_HANDLE;  // 设备2

// 设备回调函数
HDCallbackCode HDCALLBACK device1Callback(void *data);
HDCallbackCode HDCALLBACK device2Callback(void *data);

void handleKeyboard();
void printInstructions();
void initializeDevices();
void cleanupDevices();

/*******************************************************************************
 主函数
*******************************************************************************/
int main(int argc, char* argv[])
{
    // 处理命令行参数
    std::string configFile = "config.ini";  // 默认配置文件
    if (argc > 1) {
        configFile = argv[1];
        std::cout << "📄 使用指定配置文件: " << configFile << std::endl;
    } else {
        // 检查环境变量
        const char* envConfig = getenv("TOUCH_CONTROLLER_CONFIG");
        if (envConfig) {
            configFile = envConfig;
            std::cout << "📄 使用环境变量指定的配置文件: " << configFile << std::endl;
        } else {
            std::cout << "📄 使用默认配置文件: " << configFile << std::endl;
        }
    }
    
    // 创建配置加载器（启用注释功能）
    g_config = new ConfigLoader(configFile, true);

#ifdef USE_ROS2
    // 如果编译了ROS2支持，初始化ROS2
    rclcpp::init(argc, argv);
    std::cout << "✅ ROS2初始化完成" << std::endl;
#endif

    // 加载配置文件
    std::cout << "=== 加载配置文件 ===" << std::endl;
    g_config->loadConfig();
    
    // 检查是否需要保存配置文件（添加注释）
    bool autoSaveConfig = g_config->getBool("ui.auto_save_config", true);
    if (autoSaveConfig) {
        std::cout << "📝 更新配置文件（添加注释说明）..." << std::endl;
        g_config->saveConfigWithComments();
        std::cout << "✅ 配置文件已更新" << std::endl;
    }
    
    // 从配置文件获取机械臂连接参数
    std::string robot1IP = g_config->getString("robot1.ip", "192.168.10.18");
    int robot1Port = g_config->getInt("robot1.port", 8080);
    std::string robot2IP = g_config->getString("robot2.ip", "192.168.10.19");
    int robot2Port = g_config->getInt("robot2.port", 8080);
    
    std::cout << "机械臂1 IP: " << robot1IP << ", 端口: " << robot1Port << std::endl;
    std::cout << "机械臂2 IP: " << robot2IP << ", 端口: " << robot2Port << std::endl;
    
    // 创建机械臂控制器
    g_armController1 = new ArmController(robot1IP, robot1Port);
    g_armController2 = new ArmController(robot2IP, robot2Port);
    
    // 创建触觉控制器（延迟构造以便在配置文件加载后）
    g_touchArmController1 = new TouchArmController(*g_armController1, g_config, "device1");
    g_touchArmController2 = new TouchArmController(*g_armController2, g_config, "device2");

    // 连接机械臂
    std::cout << "\n=== 连接机械臂 ===" << std::endl;
    bool arm1Connected = g_armController1->connect();
    bool arm2Connected = g_armController2->connect();
    
    if (!arm1Connected && !arm2Connected) {
        std::cout << "⚠️  警告: 无法连接到任何机械臂！" << std::endl;
        std::cout << "🎮 Touch设备仍可正常工作，仅提供触觉反馈功能" << std::endl;
        std::cout << "📡 机械臂控制功能将被禁用，但所有其他功能正常" << std::endl;
        std::cout << "🔧 请检查网络连接和机械臂状态，然后重启程序以启用机械臂控制" << std::endl;
    } else {
        if (!arm1Connected) {
            std::cout << "⚠️  警告: 机械臂1连接失败，Touch设备1仅提供触觉反馈" << std::endl;
        }
        if (!arm2Connected) {
            std::cout << "⚠️  警告: 机械臂2连接失败，Touch设备2仅提供触觉反馈" << std::endl;
        }
    }

    // 初始化触觉设备
    std::cout << "\n=== 初始化触觉设备 ===" << std::endl;
    initializeDevices();

    printf("=== 双触觉设备双机械臂控制程序 ===\n");
    printf("机械臂1连接状态: %s\n", arm1Connected ? "已连接" : "未连接");
    printf("机械臂2连接状态: %s\n", arm2Connected ? "已连接" : "未连接");
    printf("\n");
    
    printInstructions();

    // 主循环
    while (g_applicationRunning)
    {
        if (_kbhit())
        {
            handleKeyboard();
        }

#ifdef USE_ROS2
        // 处理ROS2回调
        if (rclcpp::ok()) {
            rclcpp::spin_some(rclcpp::Node::make_shared("dummy_spinner"));
        }
#endif
        
        // 短暂延时
        #if defined(WIN32)
        Sleep(10);
        #else
        usleep(10000);
        #endif
    }

    // 清理工作
    cleanupDevices();
    
    // 释放配置对象内存
    delete g_config;
    g_config = nullptr;

    printf("\n程序已退出.\n");
    return 0;
}

/*******************************************************************************
 初始化两个触觉设备
*******************************************************************************/
void initializeDevices()
{
    HDErrorInfo error;
    HDSchedulerHandle hCallback1, hCallback2;

    std::cout << "=== 初始化触觉设备 ===" << std::endl;
    
    // 参考官方HelloSphereDual.cpp示例的标准初始化流程
    // 重要：所有设备实例需要在启动调度器之前创建
    
    // 初始化设备1 - 从配置文件读取设备名称
    std::string device1Primary = g_config->getString("device_names.device1_primary", "PHANToM 1");
    std::string device1Fallback = g_config->getString("device_names.device1_fallback", "Default Device");
    
    std::cout << "步骤1: 初始化设备1 (" << device1Primary << ")..." << std::endl;
    g_hHD1 = hdInitDevice(device1Primary.c_str());
    if (HD_DEVICE_ERROR(error = hdGetError())) 
    {
        std::cout << "   " << device1Primary << "未找到，尝试备用设备 (" << device1Fallback << ")..." << std::endl;
        hdGetError(); // 清除错误
        
        if (device1Fallback == "Default Device") {
            g_hHD1 = hdInitDevice(HD_DEFAULT_DEVICE);
        } else {
            g_hHD1 = hdInitDevice(device1Fallback.c_str());
        }
        
        if (HD_DEVICE_ERROR(error = hdGetError())) 
        {
            hduPrintError(stderr, &error, "无法初始化任何主设备");
            g_hHD1 = HD_INVALID_HANDLE;
        }
    }
    
    if (g_hHD1 != HD_INVALID_HANDLE) {
        hdMakeCurrentDevice(g_hHD1);
        std::cout << "✅ 发现触觉设备1: " << hdGetString(HD_DEVICE_MODEL_TYPE) 
                  << " (序列号: " << hdGetString(HD_DEVICE_SERIAL_NUMBER) << ")" << std::endl;
    }

    // 初始化设备2 - 从配置文件读取设备名称
    std::string device2Primary = g_config->getString("device_names.device2_primary", "PHANToM 2");
    std::string device2Fallbacks = g_config->getString("device_names.device2_fallback", "Device1,PHANTOM 2,PHANToM Device 2");
    
    std::cout << "步骤2: 初始化设备2 (" << device2Primary << ")..." << std::endl;
    g_hHD2 = hdInitDevice(device2Primary.c_str());
    if (HD_DEVICE_ERROR(error = hdGetError())) 
    {
        std::cout << "   " << device2Primary << "未找到，尝试备用设备..." << std::endl;
        hdGetError(); // 清除错误
        
        // 解析备用设备名称列表（逗号分隔）
        std::istringstream fallbackStream(device2Fallbacks);
        std::string deviceName;
        bool deviceFound = false;
        
        while (std::getline(fallbackStream, deviceName, ',') && !deviceFound) {
            // 移除前后空格
            deviceName.erase(0, deviceName.find_first_not_of(" \t"));
            deviceName.erase(deviceName.find_last_not_of(" \t") + 1);
            
            if (!deviceName.empty()) {
                std::cout << "   尝试设备名称: " << deviceName << std::endl;
                g_hHD2 = hdInitDevice(deviceName.c_str());
                if (!HD_DEVICE_ERROR(error = hdGetError())) {
                    deviceFound = true;
                    break; // 成功找到设备
                }
                hdGetError(); // 清除错误，继续尝试下一个
            }
        }
        
        if (!deviceFound) {
            hduPrintError(stderr, &error, "无法初始化第二个设备");
            std::cout << "⚠️  警告: 只能使用一个触觉设备" << std::endl;
            g_hHD2 = HD_INVALID_HANDLE;
        }
    }
    
    if (g_hHD2 != HD_INVALID_HANDLE) {
        hdMakeCurrentDevice(g_hHD2);
        std::cout << "✅ 发现触觉设备2: " << hdGetString(HD_DEVICE_MODEL_TYPE) 
                  << " (序列号: " << hdGetString(HD_DEVICE_SERIAL_NUMBER) << ")" << std::endl;
    }

    // 步骤3: 为每个有效设备调度回调函数（参考官方示例的顺序）
    std::cout << "步骤3: 配置设备回调函数..." << std::endl;
    
    if (g_hHD1 != HD_INVALID_HANDLE) {
        hdMakeCurrentDevice(g_hHD1);
        hCallback1 = hdScheduleAsynchronous(device1Callback, nullptr, HD_MAX_SCHEDULER_PRIORITY);
        hdEnable(HD_FORCE_OUTPUT);
        std::cout << "✅ 设备1回调函数已配置" << std::endl;
    }
    
    if (g_hHD2 != HD_INVALID_HANDLE) {
        hdMakeCurrentDevice(g_hHD2);
        hCallback2 = hdScheduleAsynchronous(device2Callback, nullptr, HD_MAX_SCHEDULER_PRIORITY);
        hdEnable(HD_FORCE_OUTPUT);
        std::cout << "✅ 设备2回调函数已配置" << std::endl;
    }

    // 步骤4: 启动调度器（参考官方示例，在所有设备初始化后统一启动）
    std::cout << "步骤4: 启动触觉调度器..." << std::endl;
    hdStartScheduler();

    // 检查错误
    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        hduPrintError(stderr, &error, "无法启动调度器");
        std::cerr << "❌ 启动调度器失败!" << std::endl;
        std::cerr << "请检查设备连接和权限" << std::endl;
        return;
    }
    
    std::cout << "✅ 触觉调度器启动成功" << std::endl;
    
    // 显示最终状态摘要
    std::cout << "\n=== 设备初始化完成 ===" << std::endl;
    std::cout << "活跃设备数量: " << ((g_hHD1 != HD_INVALID_HANDLE ? 1 : 0) + (g_hHD2 != HD_INVALID_HANDLE ? 1 : 0)) << std::endl;
    if (g_hHD1 != HD_INVALID_HANDLE) {
        std::cout << "  设备1: 已连接并激活" << std::endl;
    }
    if (g_hHD2 != HD_INVALID_HANDLE) {
        std::cout << "  设备2: 已连接并激活" << std::endl;
    }
    std::cout << "============================\n" << std::endl;
}

/*******************************************************************************
 设备1回调函数
*******************************************************************************/
HDCallbackCode HDCALLBACK device1Callback(void *data)
{
    if (g_hHD1 == HD_INVALID_HANDLE || !g_touchArmController1) {
        return HD_CALLBACK_CONTINUE;
    }

    HDErrorInfo error;
    hduVector3Dd position;
    HDdouble transform[16];
    int buttons;

    hdBeginFrame(g_hHD1);

    // 获取当前位置和姿态
    hdGetDoublev(HD_CURRENT_POSITION, position);
    hdGetDoublev(HD_CURRENT_TRANSFORM, transform);
    hdGetIntegerv(HD_CURRENT_BUTTONS, &buttons);

    // 转换格式
    std::array<double, 3> pos = {position[0], position[1], position[2]};
    std::array<double, 16> transformArray;
    for (int i = 0; i < 16; i++) {
        transformArray[i] = transform[i];
    }

    // 检测按钮状态
    bool button1Pressed = (buttons & HD_DEVICE_BUTTON_1) != 0;
    bool button2Pressed = (buttons & HD_DEVICE_BUTTON_2) != 0;
    static bool lastButton1Pressed = false;
    static bool lastButton2Pressed = false;
    
    // 按钮1控制机械臂1位置姿态
    if (button1Pressed != lastButton1Pressed) {
        if (button1Pressed) {
            g_touchArmController1->onButtonDown(pos, transformArray);
        } else {
            g_touchArmController1->onButtonUp();
        }
    }
    
    // 按钮2控制机械臂1夹抓
    if (button2Pressed != lastButton2Pressed) {
        if (button2Pressed) {
            g_touchArmController1->onGripperButtonPressed();
        }
    }
    
    // 更新机械臂1控制
    g_touchArmController1->update(pos, transformArray);
    
    lastButton1Pressed = button1Pressed;
    lastButton2Pressed = button2Pressed;

    // 应用弹簧力反馈
    hduVector3Dd force = {0.0, 0.0, 0.0};
    
    if (g_touchArmController1->isDragging()) {
        // 计算指向锚点的弹簧力
        std::array<double, 3> touchAnchor = g_touchArmController1->getTouchAnchor();
        double springStiffness = g_touchArmController1->getSpringStiffness();
        
        force[0] = springStiffness * (touchAnchor[0] - pos[0]);
        force[1] = springStiffness * (touchAnchor[1] - pos[1]);
        force[2] = springStiffness * (touchAnchor[2] - pos[2]);
        
        // 力限制
        HDdouble forceClamp;
        hdGetDoublev(HD_NOMINAL_MAX_CONTINUOUS_FORCE, &forceClamp);
        
        double forceMagnitude = sqrt(force[0]*force[0] + force[1]*force[1] + force[2]*force[2]);
        
        if (forceMagnitude > forceClamp) {
            if (forceMagnitude > 0.0) {
                double scale = forceClamp / forceMagnitude;
                force[0] *= scale;
                force[1] *= scale;
                force[2] *= scale;
            }
        }
    }
    
    hdSetDoublev(HD_CURRENT_FORCE, force);
    hdEndFrame(g_hHD1);

    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        hduPrintError(stderr, &error, "设备1回调错误");
        if (hduIsSchedulerError(&error))
            return HD_CALLBACK_DONE;
    }

    return HD_CALLBACK_CONTINUE;
}

/*******************************************************************************
 设备2回调函数
*******************************************************************************/
HDCallbackCode HDCALLBACK device2Callback(void *data)
{
    if (g_hHD2 == HD_INVALID_HANDLE || !g_touchArmController2) {
        return HD_CALLBACK_CONTINUE;
    }

    HDErrorInfo error;
    hduVector3Dd position;
    HDdouble transform[16];
    int buttons;

    hdBeginFrame(g_hHD2);

    // 获取当前位置和姿态
    hdGetDoublev(HD_CURRENT_POSITION, position);
    hdGetDoublev(HD_CURRENT_TRANSFORM, transform);
    hdGetIntegerv(HD_CURRENT_BUTTONS, &buttons);

    // 转换格式
    std::array<double, 3> pos = {position[0], position[1], position[2]};
    std::array<double, 16> transformArray;
    for (int i = 0; i < 16; i++) {
        transformArray[i] = transform[i];
    }

    // 检测按钮状态
    bool button1Pressed = (buttons & HD_DEVICE_BUTTON_1) != 0;
    bool button2Pressed = (buttons & HD_DEVICE_BUTTON_2) != 0;
    static bool lastButton1Pressed = false;
    static bool lastButton2Pressed = false;
    
    // 按钮1控制机械臂2位置姿态
    if (button1Pressed != lastButton1Pressed) {
        if (button1Pressed) {
            g_touchArmController2->onButtonDown(pos, transformArray);
        } else {
            g_touchArmController2->onButtonUp();
        }
    }
    
    // 按钮2控制机械臂2夹抓
    if (button2Pressed != lastButton2Pressed) {
        if (button2Pressed) {
            g_touchArmController2->onGripperButtonPressed();
        }
    }
    
    // 更新机械臂2控制
    g_touchArmController2->update(pos, transformArray);
    
    lastButton1Pressed = button1Pressed;
    lastButton2Pressed = button2Pressed;

    // 应用弹簧力反馈
    hduVector3Dd force = {0.0, 0.0, 0.0};
    
    if (g_touchArmController2->isDragging()) {
        // 计算指向锚点的弹簧力
        std::array<double, 3> touchAnchor = g_touchArmController2->getTouchAnchor();
        double springStiffness = g_touchArmController2->getSpringStiffness();
        
        force[0] = springStiffness * (touchAnchor[0] - pos[0]);
        force[1] = springStiffness * (touchAnchor[1] - pos[1]);
        force[2] = springStiffness * (touchAnchor[2] - pos[2]);
        
        // 力限制
        HDdouble forceClamp;
        hdGetDoublev(HD_NOMINAL_MAX_CONTINUOUS_FORCE, &forceClamp);
        
        double forceMagnitude = sqrt(force[0]*force[0] + force[1]*force[1] + force[2]*force[2]);
        
        if (forceMagnitude > forceClamp) {
            if (forceMagnitude > 0.0) {
                double scale = forceClamp / forceMagnitude;
                force[0] *= scale;
                force[1] *= scale;
                force[2] *= scale;
            }
        }
    }
    
    hdSetDoublev(HD_CURRENT_FORCE, force);
    hdEndFrame(g_hHD2);

    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        hduPrintError(stderr, &error, "设备2回调错误");
        if (hduIsSchedulerError(&error))
            return HD_CALLBACK_DONE;
    }

    return HD_CALLBACK_CONTINUE;
}

/*******************************************************************************
 清理设备资源
*******************************************************************************/
void cleanupDevices()
{
    // 停止调度器
    hdStopScheduler();

    // 禁用设备
    if (g_hHD1 != HD_INVALID_HANDLE) {
        hdDisableDevice(g_hHD1);
    }
    if (g_hHD2 != HD_INVALID_HANDLE) {
        hdDisableDevice(g_hHD2);
    }

    // 保存配置文件
    std::cout << "\n=== 保存配置文件 ===" << std::endl;
    if (g_touchArmController1) {
        g_touchArmController1->saveConfig();
    }
    
    // 清理内存
    delete g_touchArmController1;
    delete g_touchArmController2;
    delete g_armController1;
    delete g_armController2;
    
    g_touchArmController1 = nullptr;
    g_touchArmController2 = nullptr;
    g_armController1 = nullptr;
    g_armController2 = nullptr;

#ifdef USE_ROS2
    // 清理ROS2
    if (rclcpp::ok()) {
        rclcpp::shutdown();
        std::cout << "✅ ROS2清理完成" << std::endl;
    }
#endif
}

/*******************************************************************************
 键盘输入处理
*******************************************************************************/
void handleKeyboard()
{
    int key = getch();
    
    switch (key)
    {
        case 'q':
        case 'Q':
            g_applicationRunning = false;
            break;
            
        case '1':
            g_selectedDevice = 1;
            break;
            
        case '2':
            g_selectedDevice = 2;
            break;
            
        case '+':
        case '=':
            if (g_selectedDevice == 1) {
                g_touchArmController1->setPositionScale(g_touchArmController1->getPositionScale() + 100.0);
            } else if (g_selectedDevice == 2) {
                g_touchArmController2->setPositionScale(g_touchArmController2->getPositionScale() + 100.0);
            }
            break;
            
        case '-':
        case '_':
            if (g_selectedDevice == 1) {
                double newScale = g_touchArmController1->getPositionScale() - 100.0;
                if (newScale > 0.0) {  // 防止设置为负值
                    g_touchArmController1->setPositionScale(newScale);
                }
            } else if (g_selectedDevice == 2) {
                double newScale = g_touchArmController2->getPositionScale() - 100.0;
                if (newScale > 0.0) {  // 防止设置为负值
                    g_touchArmController2->setPositionScale(newScale);
                }
            }
            break;
            
        case '[':
            if (g_selectedDevice == 1) {
                double newScale = g_touchArmController1->getRotationScale() - 0.1;
                if (newScale > 0.0) {  // 防止设置为负值
                    g_touchArmController1->setRotationScale(newScale);
                }
            } else if (g_selectedDevice == 2) {
                double newScale = g_touchArmController2->getRotationScale() - 0.1;
                if (newScale > 0.0) {  // 防止设置为负值
                    g_touchArmController2->setRotationScale(newScale);
                }
            }
            break;
            
        case ']':
            if (g_selectedDevice == 1) {
                g_touchArmController1->setRotationScale(g_touchArmController1->getRotationScale() + 0.1);
            } else if (g_selectedDevice == 2) {
                g_touchArmController2->setRotationScale(g_touchArmController2->getRotationScale() + 0.1);
            }
            break;
            
        case '{':
            if (g_selectedDevice == 1) {
                g_touchArmController1->setSpringStiffness(g_touchArmController1->getSpringStiffness() * 0.9);
            } else if (g_selectedDevice == 2) {
                g_touchArmController2->setSpringStiffness(g_touchArmController2->getSpringStiffness() * 0.9);
            }
            break;
            
        case '}':
            if (g_selectedDevice == 1) {
                g_touchArmController1->setSpringStiffness(g_touchArmController1->getSpringStiffness() * 1.1);
            } else if (g_selectedDevice == 2) {
                g_touchArmController2->setSpringStiffness(g_touchArmController2->getSpringStiffness() * 1.1);
            }
            break;
            
        case 's':
        case 'S':
            if (g_selectedDevice == 1) {
                g_touchArmController1->queryCurrentArmState();
            } else if (g_selectedDevice == 2) {
                g_touchArmController2->queryCurrentArmState();
            }
            break;
            
        case 'c':
        case 'C':
            std::cout << "\n=== 保存配置到文件 ===" << std::endl;
            if (g_selectedDevice == 1) {
                g_touchArmController1->saveConfig();
            } else if (g_selectedDevice == 2) {
                g_touchArmController2->saveConfig();
            }
            break;
            
        case 'f':
        case 'F':
            {
                // 切换坐标系类型
                int currentFrameType = g_config->getInt("system.teach_frame_type", 1);
                int newFrameType = (currentFrameType == 0) ? 1 : 0;
                g_config->setInt("system.teach_frame_type", newFrameType);
                
                std::cout << "\n=== 切换坐标系类型 ===" << std::endl;
                std::cout << "从 " << (currentFrameType == 0 ? "基坐标系" : "工具坐标系") 
                          << " 切换为 " << (newFrameType == 0 ? "基坐标系" : "工具坐标系") << std::endl;
                std::cout << "注意: 需要重启程序才能生效，影响所有设备" << std::endl;
                std::cout << "========================\n" << std::endl;
            }
            break;
            
        case 'm':
        case 'M':
            {
                // 显示当前坐标映射配置
                std::cout << "\n=== 当前坐标映射配置 ===" << std::endl;
                
                // 显示设备1配置
                std::cout << "设备1 (device1_mapping):" << std::endl;
                std::cout << "  位置映射: [" 
                          << g_config->getInt("device1_mapping.touch_pos_to_arm_x", 2) << "→X, "
                          << g_config->getInt("device1_mapping.touch_pos_to_arm_y", 0) << "→Y, "
                          << g_config->getInt("device1_mapping.touch_pos_to_arm_z", 1) << "→Z]" << std::endl;
                std::cout << "  姿态映射: [" 
                          << g_config->getInt("device1_mapping.touch_rot_to_arm_rx", 2) << "→RX, "
                          << g_config->getInt("device1_mapping.touch_rot_to_arm_ry", 0) << "→RY, "
                          << g_config->getInt("device1_mapping.touch_rot_to_arm_rz", 1) << "→RZ]" << std::endl;
                std::cout << "  符号调整: [" 
                          << g_config->getInt("device1_mapping.arm_x_sign", -1) << ", "
                          << g_config->getInt("device1_mapping.arm_y_sign", -1) << ", "
                          << g_config->getInt("device1_mapping.arm_z_sign", 1) << ", "
                          << g_config->getInt("device1_mapping.arm_rx_sign", -1) << ", "
                          << g_config->getInt("device1_mapping.arm_ry_sign", -1) << ", "
                          << g_config->getInt("device1_mapping.arm_rz_sign", 1) << "]" << std::endl;
                
                std::cout << "\n设备2 (device2_mapping):" << std::endl;
                std::cout << "  位置映射: [" 
                          << g_config->getInt("device2_mapping.touch_pos_to_arm_x", 0) << "→X, "
                          << g_config->getInt("device2_mapping.touch_pos_to_arm_y", 1) << "→Y, "
                          << g_config->getInt("device2_mapping.touch_pos_to_arm_z", 2) << "→Z]" << std::endl;
                std::cout << "  姿态映射: [" 
                          << g_config->getInt("device2_mapping.touch_rot_to_arm_rx", 0) << "→RX, "
                          << g_config->getInt("device2_mapping.touch_rot_to_arm_ry", 1) << "→RY, "
                          << g_config->getInt("device2_mapping.touch_rot_to_arm_rz", 2) << "→RZ]" << std::endl;
                std::cout << "  符号调整: [" 
                          << g_config->getInt("device2_mapping.arm_x_sign", 1) << ", "
                          << g_config->getInt("device2_mapping.arm_y_sign", 1) << ", "
                          << g_config->getInt("device2_mapping.arm_z_sign", -1) << ", "
                          << g_config->getInt("device2_mapping.arm_rx_sign", 1) << ", "
                          << g_config->getInt("device2_mapping.arm_ry_sign", 1) << ", "
                          << g_config->getInt("device2_mapping.arm_rz_sign", -1) << "]" << std::endl;
                          
                std::cout << "\n说明:" << std::endl;
                std::cout << "  位置映射: 触觉设备轴索引(0=X,1=Y,2=Z) → 机械臂轴" << std::endl;
                std::cout << "  姿态映射: 触觉设备旋转轴索引(0=RX,1=RY,2=RZ) → 机械臂旋转轴" << std::endl;
                std::cout << "  符号调整: 1=正向, -1=反向" << std::endl;
                std::cout << "==========================\n" << std::endl;
            }
            break;
            
        default:
            break;
    }
}

/*******************************************************************************
 打印操作说明
*******************************************************************************/
void printInstructions()
{
    printf("=== 双设备操作说明 ===\n");
    printf("设备1 (触觉设备1): 控制机械臂1\n");
    printf("  按钮1: 控制机械臂1位置和姿态\n");
    printf("  按钮2: 控制机械臂1夹抓 (切换开/关)\n");
    printf("设备2 (触觉设备2): 控制机械臂2\n");
    printf("  按钮1: 控制机械臂2位置和姿态\n");
    printf("  按钮2: 控制机械臂2夹抓 (切换开/关)\n");
    printf("\n");
    printf("键盘控制 (实时调整):\n");
    printf("  '1'/'2': 选择调整设备1或设备2的参数 (当前: 设备%d)\n", g_selectedDevice);
    printf("  '+'/'-': 调整当前选择设备的位置映射系数\n");
    printf("  '['/']': 调整当前选择设备的姿态映射系数\n");
    printf("  '{'/'}': 调整当前选择设备的弹簧刚度\n");
    printf("  's': 查询当前选择设备的机械臂状态\n");
    printf("  'c': 保存当前选择设备的配置到文件\n");
    printf("  'f': 切换坐标系类型 (基坐标系/工具坐标系)\n");
    printf("  'm': 显示当前坐标映射配置\n");
    printf("  'q': 退出程序 (自动保存所有配置)\n");
    printf("\n");
    printf("当前参数设置:\n");
    if (g_touchArmController1) {
        printf("  设备1 - 位置映射: %.2f, 姿态映射: %.3f, 弹簧刚度: %.3f\n", 
               g_touchArmController1->getPositionScale(), 
               g_touchArmController1->getRotationScale(),
               g_touchArmController1->getSpringStiffness());
    }
    if (g_touchArmController2) {
        printf("  设备2 - 位置映射: %.2f, 姿态映射: %.3f, 弹簧刚度: %.3f\n", 
               g_touchArmController2->getPositionScale(), 
               g_touchArmController2->getRotationScale(),
               g_touchArmController2->getSpringStiffness());
    }
    printf("\n");
    printf("坐标轴映射 (支持两设备独立配置):\n");
    printf("  设备1: 使用device1_mapping节配置\n");
    printf("  设备2: 使用device2_mapping节配置\n");  
    printf("  默认位置: 触觉设备[Z,X,Y] → 机械臂[X,Y,Z]\n");
    printf("  默认姿态: 触觉设备[RZ,RX,RY] → 机械臂[-RX,-RY,RZ]\n");
    printf("  修改config.ini的device*_mapping节可自定义映射\n");
    printf("\n");
    // 从配置文件读取坐标系信息进行显示
    int frameType = g_config->getInt("system.teach_frame_type", 1);
    std::string toolName = g_config->getString("system.tool_coordinate_name", "Arm_Tip");
    
    printf("控制模式: %s控制 (%s)\n", 
           frameType == 0 ? "基坐标系" : "工具坐标系", toolName.c_str());
    printf("设备状态: %s, %s\n",
           g_hHD1 != HD_INVALID_HANDLE ? "设备1已连接" : "设备1未连接",
           g_hHD2 != HD_INVALID_HANDLE ? "设备2已连接" : "设备2未连接");
    printf("=======================================\n\n");
}
