#ifndef CONFIGLOADER_H
#define CONFIGLOADER_H

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

/**
 * @class ConfigLoader
 * @brief 配置文件加载器类，支持INI格式的配置文件读写
 * 
 * 支持功能：
 * - 读取和写入INI格式配置文件
 * - 支持注释的保存
 * - 支持不同数据类型（字符串、整数、布尔值、浮点数）
 * - 支持节和键值对的管理
 */
class ConfigLoader {
private:
    std::string m_filename;                                    // 配置文件名
    bool m_autoSave;                                          // 是否自动保存
    std::map<std::string, std::map<std::string, std::string>> m_config;  // 配置数据
    std::map<std::string, std::vector<std::string>> m_comments;          // 注释数据
    
    /**
     * @brief 去除字符串首尾空白字符
     * @param str 输入字符串
     * @return 处理后的字符串
     */
    std::string trim(const std::string& str);
    
    /**
     * @brief 解析配置行
     * @param line 配置行内容
     * @param currentSection 当前节名
     * @return 是否解析成功
     */
    bool parseLine(const std::string& line, std::string& currentSection);

public:
    /**
     * @brief 构造函数
     * @param filename 配置文件路径
     * @param autoSave 是否自动保存（默认false）
     */
    ConfigLoader(const std::string& filename, bool autoSave = false);
    
    /**
     * @brief 析构函数
     */
    ~ConfigLoader();
    
    /**
     * @brief 加载配置文件
     * @return 是否加载成功
     */
    bool loadConfig();
    
    /**
     * @brief 保存配置文件（带注释）
     * @return 是否保存成功
     */
    bool saveConfigWithComments();
    
    /**
     * @brief 保存配置文件（不带注释）
     * @return 是否保存成功
     */
    bool saveConfig();
    
    /**
     * @brief 获取字符串值
     * @param key 键名（格式：section.key）
     * @param defaultValue 默认值
     * @return 配置值
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "");
    
    /**
     * @brief 获取整数值
     * @param key 键名（格式：section.key）
     * @param defaultValue 默认值
     * @return 配置值
     */
    int getInt(const std::string& key, int defaultValue = 0);
    
    /**
     * @brief 获取布尔值
     * @param key 键名（格式：section.key）
     * @param defaultValue 默认值
     * @return 配置值
     */
    bool getBool(const std::string& key, bool defaultValue = false);
    
    /**
     * @brief 获取浮点数值
     * @param key 键名（格式：section.key）
     * @param defaultValue 默认值
     * @return 配置值
     */
    double getDouble(const std::string& key, double defaultValue = 0.0);
    
    /**
     * @brief 设置字符串值
     * @param key 键名（格式：section.key）
     * @param value 值
     */
    void setString(const std::string& key, const std::string& value);
    
    /**
     * @brief 设置整数值
     * @param key 键名（格式：section.key）
     * @param value 值
     */
    void setInt(const std::string& key, int value);
    
    /**
     * @brief 设置布尔值
     * @param key 键名（格式：section.key）
     * @param value 值
     */
    void setBool(const std::string& key, bool value);
    
    /**
     * @brief 设置浮点数值
     * @param key 键名（格式：section.key）
     * @param value 值
     */
    void setDouble(const std::string& key, double value);
    
    /**
     * @brief 检查键是否存在
     * @param key 键名（格式：section.key）
     * @return 是否存在
     */
    bool hasKey(const std::string& key);
    
    /**
     * @brief 获取所有节名
     * @return 节名列表
     */
    std::vector<std::string> getSections();
    
    /**
     * @brief 获取指定节的所有键名
     * @param section 节名
     * @return 键名列表
     */
    std::vector<std::string> getKeys(const std::string& section);
    
    /**
     * @brief 设置注释
     * @param section 节名
     * @param comments 注释列表
     */
    void setComments(const std::string& section, const std::vector<std::string>& comments);
    
    /**
     * @brief 添加注释
     * @param section 节名
     * @param comment 注释内容
     */
    void addComment(const std::string& section, const std::string& comment);
};

#endif // CONFIGLOADER_H
