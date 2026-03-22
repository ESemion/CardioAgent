#include "Config.h"
#include "Logger.h"
#include <fstream>

Config::Config() : m_pollInterval(10) {}

bool Config::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        Logger::instance().error("Не удалось открыть файл конфигурации: " + filepath);
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;
        
        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);
        
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        if (key == "UID") m_uid = value;
        else if (key == "descr") m_description = value;
        else if (key == "server_url") m_serverUrl = value;
        else if (key == "poll_interval") {
            try {
                m_pollInterval = std::stoi(value);
            } catch (...) {
                Logger::instance().error("Некорректное значение poll_interval");
                return false;
            }
        }
    }
    
    if (m_uid.empty()) {
        Logger::instance().error("В конфигурации не найден параметр UID");
        return false;
    }
    if (m_serverUrl.empty()) {
        Logger::instance().error("В конфигурации не найден параметр server_url");
        return false;
    }
    
    Logger::instance().info("Конфигурация загружена: UID=" + m_uid);
    return true;
}

std::string Config::getUid() const { return m_uid; }
std::string Config::getDescription() const { return m_description; }
std::string Config::getServerUrl() const { return m_serverUrl; }
int Config::getPollInterval() const { return m_pollInterval; }