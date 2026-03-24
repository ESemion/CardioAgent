#include "Config.h"
#include "Logger.h"
#include <fstream>
#include <ctime>

Config::Config() : m_pollInterval(10), m_maxPollInterval(300) {}

bool Config::save(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    file << "UID=" << m_uid << std::endl;
    file << "descr=" << m_description << std::endl;
    file << "server_url=" << m_serverUrl << std::endl;
    file << "poll_interval=" << std::to_string(m_pollInterval) << std::endl;
    file << "max_poll_interval=" << std::to_string(m_maxPollInterval) << std::endl;
    file << "access_code=" << m_accessCode << std::endl;

    
    return true;
}


bool Config::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        Logger::instance().error("Не удалось открыть файл конфигурации: " + filepath);
        return false;
    }
    
    bool needResaving = false;
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
        else if (key == "access_code") m_accessCode = value;
        else if (key == "descr") m_description = value;
        else if (key == "server_url") m_serverUrl = value;
        else if (key == "poll_interval") {
            try {
                m_pollInterval = std::stoi(value);
            } catch (...) {
                m_pollInterval = 10;
                needResaving = true;
            }
        }
        else if (key == "max_poll_interval") {
            try {
                m_maxPollInterval = std::stoi(value);
            } catch (...) {
                m_maxPollInterval = 300;
                needResaving = true;
            }
        }
    }

    if (m_uid.empty()) {
        // Генерируем уникальный ID 
        m_uid = "agent_" + std::to_string(time(nullptr));
        m_accessCode = "";
        needResaving = true;
        

    }
    if (m_serverUrl.empty()) {
        Logger::instance().error("В конфигурации не найден параметр server_url");
        return false;
    }

    if (m_description.empty()) {
        m_description = "agent";
        needResaving = true;
    }
    if (needResaving) {
        save("config/agent.ini");  
    }
    Logger::instance().info("Конфигурация загружена: UID=" + m_uid);
    return true;
}

std::string Config::getUid() const { return m_uid; }
std::string Config::getDescription() const { return m_description; }
std::string Config::getServerUrl() const { return m_serverUrl; }
int Config::getPollInterval() const { return m_pollInterval; }
int Config::getMaxPollInterval() const { return m_maxPollInterval; }
std::string Config::getAccessCode() const { return m_accessCode; }

void Config::setAccessCode(const std::string& code) { m_accessCode = code; }