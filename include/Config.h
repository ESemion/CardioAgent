#ifndef CONFIG_H
#define CONFIG_H

#include <string>

/**
 * Класс для чтения настроек из INI-файла
 * Читает параметры: UID, descr, server_url, poll_interval
 */
class Config {
private:
    std::string m_uid;           // ID агента (например "007")
    std::string m_description;   // Описание агента
    std::string m_serverUrl;     // Адрес сервера
    int m_pollInterval;          // Интервал опроса в секундах

public:
    Config();
    
    /**
     * Загружает настройки из файла
     * @param filepath путь к файлу (например "config/agent.ini")
     * @return true если успешно, false при ошибке
     */
    bool load(const std::string& filepath);
    
    // Геттеры
    std::string getUid() const;
    std::string getDescription() const;
    std::string getServerUrl() const;
    int getPollInterval() const;
};

#endif // CONFIG_H