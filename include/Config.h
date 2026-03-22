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
     * @brief Сохранение конфигурации в файл
     * @param filepath путь к INI-файлу
     * @return true при успешном сохранении
     */
    bool save(const std::string& filepath);
    
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
    std::string getAccessCode() const;   ///< получить код доступа

    void setAccessCode(const std::string& code);  ///< установить код доступа
    
private:
    std::string m_uid;          ///< идентификатор агента
    std::string m_description;  ///< описание агента
    std::string m_serverUrl;    ///< адрес сервера
    std::string m_accessCode;   ///< код доступа от сервера
    int m_pollInterval;         ///< интервал опроса в секундах
};

#endif