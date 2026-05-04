#include "Logger.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : m_level(LogLevel::INFO) {}

Logger::~Logger() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void Logger::setLogFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    m_logFile.open(filepath, std::ios::app);
}

void Logger::setLevel(LogLevel level) {
    m_level = level;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < m_level) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string formatted = currentTimestamp() + " [" + levelToString(level) + "] " + message;
    
    if (m_logFile.is_open()) {
        m_logFile << formatted << std::endl;
    }
    
        std::cerr << formatted << std::endl;
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Logger::currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    #ifdef _WIN32
        localtime_s(&tm, &time);
    #else
        localtime_r(&time, &tm);
    #endif
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}