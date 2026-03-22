#ifndef LOGGER_H
#define LOGGER_H

#include <string>

// Библиотеки для вывода логов в файл
#include <fstream>
#include <mutex>

/**
 * @brief Уровни логирования
 */
enum class LogLevel {
    DEBUG,   ///< отладочные сообщения
    INFO,    ///< информационные сообщения
    WARNING, ///< предупреждения
    ERROR    ///< ошибки
};

/**
 * @class Logger
 * @brief Потокобезопасный логгер с записью в файл
 */
class Logger {
public:
    /**
     * @brief Получение экземпляра синглтона
     * @return ссылка на единственный экземпляр
     */
    static Logger& instance();
    
    void setLogFile(const std::string& filepath); ///< установить файл лога
    void setLevel(LogLevel level);                ///< установить уровень логирования
    
    void debug(const std::string& message);   ///< отладочное сообщение
    void info(const std::string& message);    ///< информационное сообщение
    void warning(const std::string& message); ///< предупреждение
    void error(const std::string& message);   ///< сообщение об ошибке
    
private:
    Logger();
    ~Logger();
    
    void log(LogLevel level, const std::string& message);
    std::string levelToString(LogLevel level);
    std::string currentTimestamp();
    
    std::ofstream m_logFile;  ///< файловый поток
    LogLevel m_level;         ///< текущий уровень логирования
    std::mutex m_mutex;       ///< мьютекс для потокобезопасности
};

#endif