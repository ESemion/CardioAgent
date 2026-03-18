#include "Config.h"
#include <fstream>
#include <iostream>
#include <algorithm>

// Удаляет пробелы в начале и конце строки
static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

Config::Config() 
    : m_uid("")
    , m_description("")
    , m_serverUrl("")
    , m_pollInterval(10) 
{}

bool Config::load(const std::string& filepath) {
    std::cout << "Читаем файл: " << filepath << std::endl;
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Ошибка: файл не найден" << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == ';') continue;
        
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;
        
        std::string key = trim(line.substr(0, eqPos));
        std::string value = trim(line.substr(eqPos + 1));
        
        if (key == "UID") {
            m_uid = value;
        } else if (key == "descr") {
            m_description = value;
        } else if (key == "server_url") {
            m_serverUrl = value;
        } else if (key == "poll_interval") {
            try {
                m_pollInterval = std::stoi(value);
            } catch (...) {
                std::cerr << "Ошибка: poll_interval должно быть числом" << std::endl;
                return false;
            }
        }
    }
    
    // Проверка обязательных полей
    if (m_uid.empty()) {
        std::cerr << "Ошибка: не найден UID" << std::endl;
        return false;
    }
    if (m_serverUrl.empty()) {
        std::cerr << "Ошибка: не найден server_url" << std::endl;
        return false;
    }
    
    return true;
}

std::string Config::getUid() const { return m_uid; }
std::string Config::getDescription() const { return m_description; }
std::string Config::getServerUrl() const { return m_serverUrl; }
int Config::getPollInterval() const { return m_pollInterval; }