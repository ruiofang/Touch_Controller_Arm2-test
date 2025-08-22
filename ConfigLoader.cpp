#include "ConfigLoader.h"
#include <algorithm>
#include <cctype>

ConfigLoader::ConfigLoader(const std::string& filename, bool autoSave) 
    : m_filename(filename), m_autoSave(autoSave) {
    // 构造函数中可以选择是否立即加载配置
    if (!filename.empty()) {
        loadConfig();
    }
}

ConfigLoader::~ConfigLoader() {
    if (m_autoSave) {
        saveConfigWithComments();
    }
}

std::string ConfigLoader::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool ConfigLoader::parseLine(const std::string& line, std::string& currentSection) {
    std::string trimmedLine = trim(line);
    
    // 空行或注释行
    if (trimmedLine.empty() || trimmedLine[0] == '#' || trimmedLine[0] == ';') {
        if (trimmedLine[0] == '#' || trimmedLine[0] == ';') {
            // 保存注释
            addComment(currentSection, trimmedLine);
        }
        return true;
    }
    
    // 节标题
    if (trimmedLine[0] == '[' && trimmedLine.back() == ']') {
        currentSection = trimmedLine.substr(1, trimmedLine.length() - 2);
        currentSection = trim(currentSection);
        // 确保节存在
        if (m_config.find(currentSection) == m_config.end()) {
            m_config[currentSection] = std::map<std::string, std::string>();
        }
        return true;
    }
    
    // 键值对
    size_t equalPos = trimmedLine.find('=');
    if (equalPos != std::string::npos) {
        std::string key = trim(trimmedLine.substr(0, equalPos));
        std::string value = trim(trimmedLine.substr(equalPos + 1));
        
        if (!currentSection.empty() && !key.empty()) {
            m_config[currentSection][key] = value;
            return true;
        }
    }
    
    return false;
}

bool ConfigLoader::loadConfig() {
    std::ifstream file(m_filename);
    if (!file.is_open()) {
        std::cerr << "警告：无法打开配置文件 " << m_filename << "，将使用默认配置" << std::endl;
        return false;
    }
    
    std::string line;
    std::string currentSection;
    
    // 清空现有配置
    m_config.clear();
    m_comments.clear();
    
    while (std::getline(file, line)) {
        parseLine(line, currentSection);
    }
    
    file.close();
    return true;
}

bool ConfigLoader::saveConfigWithComments() {
    std::ofstream file(m_filename);
    if (!file.is_open()) {
        std::cerr << "错误：无法写入配置文件 " << m_filename << std::endl;
        return false;
    }
    
    // 写入文件头注释
    file << "# Touch Controller Arm 配置文件" << std::endl;
    file << "# 自动生成，请谨慎修改" << std::endl;
    file << std::endl;
    
    // 按节写入配置
    for (const auto& section : m_config) {
        const std::string& sectionName = section.first;
        const auto& keyValues = section.second;
        
        // 写入节前的注释
        if (m_comments.find(sectionName) != m_comments.end()) {
            for (const auto& comment : m_comments[sectionName]) {
                file << comment << std::endl;
            }
        }
        
        // 写入节标题
        file << "[" << sectionName << "]" << std::endl;
        
        // 写入键值对
        for (const auto& kv : keyValues) {
            file << kv.first << " = " << kv.second << std::endl;
        }
        
        file << std::endl;  // 节之间空行
    }
    
    file.close();
    return true;
}

bool ConfigLoader::saveConfig() {
    std::ofstream file(m_filename);
    if (!file.is_open()) {
        std::cerr << "错误：无法写入配置文件 " << m_filename << std::endl;
        return false;
    }
    
    // 按节写入配置（不包含注释）
    for (const auto& section : m_config) {
        const std::string& sectionName = section.first;
        const auto& keyValues = section.second;
        
        file << "[" << sectionName << "]" << std::endl;
        
        for (const auto& kv : keyValues) {
            file << kv.first << " = " << kv.second << std::endl;
        }
        
        file << std::endl;
    }
    
    file.close();
    return true;
}

std::string ConfigLoader::getString(const std::string& key, const std::string& defaultValue) {
    size_t dotPos = key.find('.');
    if (dotPos == std::string::npos) {
        return defaultValue;
    }
    
    std::string section = key.substr(0, dotPos);
    std::string keyName = key.substr(dotPos + 1);
    
    auto sectionIt = m_config.find(section);
    if (sectionIt != m_config.end()) {
        auto keyIt = sectionIt->second.find(keyName);
        if (keyIt != sectionIt->second.end()) {
            return keyIt->second;
        }
    }
    
    // 如果键不存在，设置默认值
    setString(key, defaultValue);
    return defaultValue;
}

int ConfigLoader::getInt(const std::string& key, int defaultValue) {
    std::string value = getString(key, std::to_string(defaultValue));
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        setInt(key, defaultValue);
        return defaultValue;
    }
}

bool ConfigLoader::getBool(const std::string& key, bool defaultValue) {
    std::string value = getString(key, defaultValue ? "true" : "false");
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return true;
    } else if (value == "false" || value == "0" || value == "no" || value == "off") {
        return false;
    } else {
        setBool(key, defaultValue);
        return defaultValue;
    }
}

double ConfigLoader::getDouble(const std::string& key, double defaultValue) {
    std::string value = getString(key, std::to_string(defaultValue));
    try {
        return std::stod(value);
    } catch (const std::exception&) {
        setDouble(key, defaultValue);
        return defaultValue;
    }
}

void ConfigLoader::setString(const std::string& key, const std::string& value) {
    size_t dotPos = key.find('.');
    if (dotPos == std::string::npos) {
        return;
    }
    
    std::string section = key.substr(0, dotPos);
    std::string keyName = key.substr(dotPos + 1);
    
    // 确保节存在
    if (m_config.find(section) == m_config.end()) {
        m_config[section] = std::map<std::string, std::string>();
    }
    
    m_config[section][keyName] = value;
    
    if (m_autoSave) {
        saveConfigWithComments();
    }
}

void ConfigLoader::setInt(const std::string& key, int value) {
    setString(key, std::to_string(value));
}

void ConfigLoader::setBool(const std::string& key, bool value) {
    setString(key, value ? "true" : "false");
}

void ConfigLoader::setDouble(const std::string& key, double value) {
    setString(key, std::to_string(value));
}

bool ConfigLoader::hasKey(const std::string& key) {
    size_t dotPos = key.find('.');
    if (dotPos == std::string::npos) {
        return false;
    }
    
    std::string section = key.substr(0, dotPos);
    std::string keyName = key.substr(dotPos + 1);
    
    auto sectionIt = m_config.find(section);
    if (sectionIt != m_config.end()) {
        return sectionIt->second.find(keyName) != sectionIt->second.end();
    }
    
    return false;
}

std::vector<std::string> ConfigLoader::getSections() {
    std::vector<std::string> sections;
    for (const auto& section : m_config) {
        sections.push_back(section.first);
    }
    return sections;
}

std::vector<std::string> ConfigLoader::getKeys(const std::string& section) {
    std::vector<std::string> keys;
    auto sectionIt = m_config.find(section);
    if (sectionIt != m_config.end()) {
        for (const auto& kv : sectionIt->second) {
            keys.push_back(kv.first);
        }
    }
    return keys;
}

void ConfigLoader::setComments(const std::string& section, const std::vector<std::string>& comments) {
    m_comments[section] = comments;
}

void ConfigLoader::addComment(const std::string& section, const std::string& comment) {
    m_comments[section].push_back(comment);
}
