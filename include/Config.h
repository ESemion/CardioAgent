#ifndef CONFIG_H
#define CONFIG_H

#include <string>

/**
 * @class Config
 * @brief Загрузка и хранение настроек из INI-файла
 */
class Config {
public:
    Config();

    bool save(const std::string& filepath);

    /**
     * @brief Загрузка конфигурации из INI-файла
     */
    bool load(const std::string& filepath);

    std::string generateUID();

    std::string getUid() const;
    std::string getDescription() const;
    std::string getServerUrl() const;
    int getPollInterval() const;
    int getMaxPollInterval() const;
    std::string getAccessCode() const;
    
    // Новые геттеры
    std::string getResultPath() const;
    std::string getProgramPath() const;

    void setAccessCode(const std::string& code);
    void setDescription(const std::string& desc);
    void setPollInterval(int interval);
    
    // Новые сеттеры
    void setResultPath(const std::string& path);
    void setProgramPath(const std::string& path);

private:
    std::string m_uid;
    std::string m_description;
    std::string m_serverUrl;
    std::string m_accessCode;
    int m_pollInterval;
    int m_maxPollInterval;
    
    // Новые поля
    std::string m_resultPath;
    std::string m_programPath;
};

#endif