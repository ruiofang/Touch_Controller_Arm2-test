/*****************************************************************************
 灵巧手集成测试程序
 
 功能：
 - 测试灵巧手控制器是否能正确初始化
 - 测试配置文件中的动作是否能正确执行
 - 验证Python集成是否正常工作
*****************************************************************************/

#include <Python.h>
#include <iostream>
#include <string>
#include "ConfigLoader.h"

// 简化版的灵巧手控制器，用于测试
class TestHandController {
private:
    std::string m_handType;
    std::string m_handJoint;
    std::string m_canInterface;
    std::string m_graspAction;
    std::string m_releaseAction;
    bool m_pythonInitialized;
    PyObject* m_handModule;
    PyObject* m_yamlLoader;
    
public:
    TestHandController(const std::string& handType = "left", 
                      const std::string& handJoint = "L7",
                      const std::string& canInterface = "can0",
                      const std::string& graspAction = "ZQ",
                      const std::string& releaseAction = "张开")
        : m_handType(handType), m_handJoint(handJoint), m_canInterface(canInterface),
          m_graspAction(graspAction), m_releaseAction(releaseAction),
          m_pythonInitialized(false), m_handModule(nullptr), m_yamlLoader(nullptr) {
    }
    
    ~TestHandController() {
        cleanup();
    }
    
    bool initialize() {
        try {
            // 初始化Python环境
            if (!Py_IsInitialized()) {
                Py_Initialize();
                if (!Py_IsInitialized()) {
                    std::cerr << "❌ Python环境初始化失败" << std::endl;
                    return false;
                }
            }
            
            // 添加SDK路径
            PyRun_SimpleString("import sys");
            std::string sdkPath = "sys.path.append('/home/ruio/Touch_Controller_Arm2-test/linker_hand_python_sdk')";
            PyRun_SimpleString(sdkPath.c_str());
            
            // 导入YAML加载器
            PyObject* yamlModule = PyImport_ImportModule("LinkerHand.utils.load_write_yaml");
            if (!yamlModule) {
                PyErr_Print();
                std::cerr << "❌ 无法导入YAML加载器模块" << std::endl;
                return false;
            }
            
            PyObject* yamlClass = PyObject_GetAttrString(yamlModule, "LoadWriteYaml");
            if (!yamlClass) {
                PyErr_Print();
                std::cerr << "❌ 无法获取LoadWriteYaml类" << std::endl;
                Py_DECREF(yamlModule);
                return false;
            }
            
            PyObject* yamlArgs = PyTuple_New(0);
            m_yamlLoader = PyObject_Call(yamlClass, yamlArgs, nullptr);
            
            Py_DECREF(yamlArgs);
            Py_DECREF(yamlClass);
            Py_DECREF(yamlModule);
            
            if (!m_yamlLoader) {
                PyErr_Print();
                std::cerr << "❌ 无法创建YAML加载器实例" << std::endl;
                return false;
            }
            
            m_pythonInitialized = true;
            std::cout << "✅ 测试控制器初始化成功: " << m_handType << " " << m_handJoint << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "❌ 初始化异常: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testLoadAction(const std::string& actionName) {
        if (!m_pythonInitialized || !m_yamlLoader) {
            std::cerr << "❌ 控制器未初始化" << std::endl;
            return false;
        }
        
        try {
            std::cout << "🔍 测试加载动作: " << actionName << std::endl;
            
            // 从YAML文件加载动作
            PyObject* loadActionMethod = PyObject_GetAttrString(m_yamlLoader, "load_action_yaml");
            if (!loadActionMethod) {
                std::cerr << "❌ 无法获取load_action_yaml方法" << std::endl;
                return false;
            }
            
            PyObject* loadArgs = PyTuple_New(0);
            PyObject* loadKwargs = PyDict_New();
            PyDict_SetItemString(loadKwargs, "hand_joint", PyUnicode_FromString(m_handJoint.c_str()));
            PyDict_SetItemString(loadKwargs, "hand_type", PyUnicode_FromString(m_handType.c_str()));
            
            PyObject* actionsData = PyObject_Call(loadActionMethod, loadArgs, loadKwargs);
            
            Py_DECREF(loadArgs);
            Py_DECREF(loadKwargs);
            Py_DECREF(loadActionMethod);
            
            if (!actionsData || actionsData == Py_None) {
                std::cerr << "❌ 无法加载动作配置数据" << std::endl;
                return false;
            }
            
            // 查找指定的动作
            PyObject* actionPosition = nullptr;
            bool found = false;
            if (PyList_Check(actionsData)) {
                Py_ssize_t listSize = PyList_Size(actionsData);
                std::cout << "📋 找到 " << listSize << " 个动作配置" << std::endl;
                
                for (Py_ssize_t i = 0; i < listSize; i++) {
                    PyObject* action = PyList_GetItem(actionsData, i);
                    if (PyDict_Check(action)) {
                        PyObject* nameObj = PyDict_GetItemString(action, "ACTION_NAME");
                        if (nameObj && PyUnicode_Check(nameObj)) {
                            const char* name = PyUnicode_AsUTF8(nameObj);
                            std::cout << "   动作 " << (i+1) << ": " << name << std::endl;
                            if (name && actionName == name) {
                                actionPosition = PyDict_GetItemString(action, "POSITION");
                                found = true;
                                std::cout << "✅ 找到动作: " << actionName << std::endl;
                                
                                // 显示位置数据
                                if (actionPosition && PyList_Check(actionPosition)) {
                                    Py_ssize_t posSize = PyList_Size(actionPosition);
                                    std::cout << "   位置数据 (" << posSize << " 个关节): ";
                                    for (Py_ssize_t j = 0; j < posSize; j++) {
                                        PyObject* posItem = PyList_GetItem(actionPosition, j);
                                        if (PyFloat_Check(posItem)) {
                                            std::cout << PyFloat_AsDouble(posItem);
                                        } else if (PyLong_Check(posItem)) {
                                            std::cout << PyLong_AsLong(posItem);
                                        }
                                        if (j < posSize - 1) std::cout << ", ";
                                    }
                                    std::cout << std::endl;
                                }
                            }
                        }
                    }
                }
            }
            
            Py_DECREF(actionsData);
            
            if (!found) {
                std::cerr << "❌ 未找到动作: " << actionName << std::endl;
                return false;
            }
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "❌ 测试异常: " << e.what() << std::endl;
            return false;
        }
    }
    
private:
    void cleanup() {
        if (m_yamlLoader) {
            Py_DECREF(m_yamlLoader);
            m_yamlLoader = nullptr;
        }
        
        if (m_handModule) {
            Py_DECREF(m_handModule);
            m_handModule = nullptr;
        }
        
        m_pythonInitialized = false;
    }
};

int main() {
    std::cout << "=== 灵巧手集成测试 ===" << std::endl;
    
    // 加载配置文件
    ConfigLoader config("config.ini");
    if (!config.loadConfig()) {
        std::cerr << "❌ 无法加载配置文件 config.ini" << std::endl;
        return 1;
    }
    
    std::cout << "✅ 配置文件加载成功" << std::endl;
    
    // 测试设备1（左手）
    std::cout << "\n--- 测试设备1（左手）---" << std::endl;
    std::string handType1 = config.getString("device1_mapping.hand_type", "left");
    std::string handJoint1 = config.getString("device1_mapping.hand_joint", "L7");
    std::string canInterface1 = config.getString("device1_mapping.can_interface", "can0");
    std::string graspAction1 = config.getString("device1_mapping.grasp_action", "ZQ");
    std::string releaseAction1 = config.getString("device1_mapping.release_action", "张开");
    
    TestHandController hand1(handType1, handJoint1, canInterface1, graspAction1, releaseAction1);
    
    if (hand1.initialize()) {
        std::cout << "✅ 设备1初始化成功" << std::endl;
        hand1.testLoadAction(graspAction1);
        hand1.testLoadAction(releaseAction1);
    } else {
        std::cerr << "❌ 设备1初始化失败" << std::endl;
    }
    
    // 测试设备2（右手）
    std::cout << "\n--- 测试设备2（右手）---" << std::endl;
    std::string handType2 = config.getString("device2_mapping.hand_type", "right");
    std::string handJoint2 = config.getString("device2_mapping.hand_joint", "L7");
    std::string canInterface2 = config.getString("device2_mapping.can_interface", "can1");
    std::string graspAction2 = config.getString("device2_mapping.grasp_action", "ZQ");
    std::string releaseAction2 = config.getString("device2_mapping.release_action", "张开");
    
    TestHandController hand2(handType2, handJoint2, canInterface2, graspAction2, releaseAction2);
    
    if (hand2.initialize()) {
        std::cout << "✅ 设备2初始化成功" << std::endl;
        hand2.testLoadAction(graspAction2);
        hand2.testLoadAction(releaseAction2);
    } else {
        std::cerr << "❌ 设备2初始化失败" << std::endl;
    }
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    
    return 0;
}
