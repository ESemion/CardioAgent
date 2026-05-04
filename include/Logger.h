#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

#ifdef ERROR
#undef ERROR
#endif
#ifdef DEBUG
#undef DEBUG
#endif

/// @brief Уровни логирования (фильтруются через Logger::setLevel)
enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

/**
 * @class Logger
 * @brief Потокобезопасный синглтон-логгер с записью в файл и stderr
 */
class Logger {
public:
    static Logger& instance();

    void setLogFile(const std::string& filepath);
    void setLevel(LogLevel level);

    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);

private:
    Logger();
    ~Logger();

    void log(LogLevel level, const std::string& message);
    std::string levelToString(LogLevel level);
    std::string currentTimestamp();

    std::ofstream m_logFile;
    LogLevel m_level;
    std::mutex m_mutex;
};

#endif
