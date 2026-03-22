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
    
    /**
     * @brief Загрузка конфигурации из файла
     * @param filepath путь к INI-файлу
     * @return true при успешной загрузке
     */
    bool load(const std::string& filepath);
    
    std::string getUid() const;          ///< получить UID агента
    std::string getDescription() const;  ///< получить описание
    std::string getServerUrl() const;    ///< получить URL сервера
    int getPollInterval() const;         ///< получить интервал опроса
    
private:
    std::string m_uid;          ///< идентификатор агента
    std::string m_description;  ///< описание агента
    std::string m_serverUrl;    ///< адрес сервера
    int m_pollInterval;         ///< интервал опроса в секундах
};

#endif