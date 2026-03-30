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
     *
     * Автоматически генерирует UID при первом запуске и пересохраняет
     * файл, если были подставлены значения по умолчанию.
     */
    bool load(const std::string& filepath);

    std::string getUid() const;
    std::string getDescription() const;
    std::string getServerUrl() const;
    int getPollInterval() const;
    int getMaxPollInterval() const;
    std::string getAccessCode() const;

    void setAccessCode(const std::string& code);

private:
    std::string m_uid;
    std::string m_description;
    std::string m_serverUrl;
    std::string m_accessCode;
    int m_pollInterval;
    int m_maxPollInterval;
};

#endif
