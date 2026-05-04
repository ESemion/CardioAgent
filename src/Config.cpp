#include "Config.h"
#include "Logger.h"
#include <fstream>
#include <ctime>
#include <random>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

// ... метод generateUID остается без изменений ...
std::string Config::generateUID() {
    char hostname[256];
    #ifdef _WIN32
        DWORD size = sizeof(hostname);
        GetComputerNameA(hostname, &size);
    #else
        gethostname(hostname, sizeof(hostname));
    #endif
    
    static const char hex[] = "0123456789abcdef";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::string uid = std::string(hostname) + "_";
    for (int i = 0; i < 8; i++) {
        uid += hex[gen() % 16];
    }
    return uid;
}

Config::Config() : m_pollInterval(10), m_maxPollInterval(300) {
    // Можно задать значения по умолчанию в конструкторе
    m_resultPath = "results";
    m_programPath = "programs";
}

bool Config::save(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    
    file << "UID=" << m_uid << std::endl;
    file << "descr=" << m_description << std::endl;
    file << "server_url=" << m_serverUrl << std::endl;
    file << "poll_interval=" << std::to_string(m_pollInterval) << std::endl;
    file << "max_poll_interval=" << std::to_string(m_maxPollInterval) << std::endl;
    file << "access_code=" << m_accessCode << std::endl;
    
    // Сохранение новых полей
    file << "result_path=" << m_resultPath << std::endl;
    file << "program_path=" << m_programPath << std::endl;

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
        // Чтение новых полей
        else if (key == "result_path") m_resultPath = value;
        else if (key == "program_path") m_programPath = value;
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

    // Проверка на пустые значения для новых полей и установка дефолтов
    if (m_uid.empty()) {
        m_uid = generateUID();
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

    // Обработка дефолтов для путей, если их не было в файле
    if (m_resultPath.empty()) {
        m_resultPath = "results";
        needResaving = true;
    }
    if (m_programPath.empty()) {
        m_programPath = "programs";
        needResaving = true;
    }

    if (needResaving) {
        if (!save(filepath)) { // Используем переданный filepath вместо хардкода
            Logger::instance().error("Не удалось пересохранить конфиг");
        }
    }
    
    Logger::instance().info("Конфигурация загружена: UID=" + m_uid);
    return true;
}

// Старые геттеры
std::string Config::getUid() const { return m_uid; }
std::string Config::getDescription() const { return m_description; }
std::string Config::getServerUrl() const { return m_serverUrl; }
int Config::getPollInterval() const { return m_pollInterval; }
int Config::getMaxPollInterval() const { return m_maxPollInterval; }
std::string Config::getAccessCode() const { return m_accessCode; }

// Реализация новых геттеров
std::string Config::getResultPath() const { return m_resultPath; }
std::string Config::getProgramPath() const { return m_programPath; }

// Старые сеттеры
void Config::setAccessCode(const std::string& code) { m_accessCode = code; }
void Config::setDescription(const std::string& desc) { m_description = desc; }
void Config::setPollInterval(int interval) { m_pollInterval = interval; }

// Реализация новых сеттеров
void Config::setResultPath(const std::string& path) { m_resultPath = path; }
void Config::setProgramPath(const std::string& path) { m_programPath = path; }