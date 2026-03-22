#ifndef AGENT_H
#define AGENT_H

//Макрос для поддержки OpenSSL
#define CPPHTTPLIB_OPENSSL_SUPPORT 1

#include "Task.h"
#include "Config.h"
#include <string>
#include <memory>
#include <functional>

// Библиотеки для работы с потоками
#include <thread>
#include <atomic>

// Библиотека для работы с HTTP
#include <httplib.h>

/**
 * @class Agent
 * @brief Управляет регистрацией и опросом сервера
 */
class Agent {
public:
    /**
     * @brief Конструктор
     * @param config объект конфигурации
     */
    Agent(const Config& config);
    ~Agent();
    
    /**
     * @brief Регистрация агента на сервере
     * @return true при успешной регистрации
     */
    bool registerAgent();
    
    /**
     * @brief Запуск цикла опроса сервера
     * @param callback функция обработки полученных заданий
     */
    void start(std::function<ExecutionResult(const Task&)> callback);
    
    /**
     * @brief Остановка цикла опроса
     */
    void stop();
    
    /**
     * @brief Отправка результатов выполнения на сервер
     * @param sessionId идентификатор сессии задания
     * @param result результат выполнения
     * @return true при успешной отправке
     */
    bool uploadResults(const std::string& sessionId, const ExecutionResult& result);
    
private:
    Config m_config;                           ///< настройки агента
    std::string m_accessCode;                  ///< код доступа от сервера
    std::unique_ptr<httplib::SSLClient> m_httpClient; ///< HTTP клиент
    std::atomic<bool> m_running;               ///< флаг работы
    std::thread m_pollThread;                  ///< поток опроса
    
    /**
     * @brief Извлечение значения по ключу из JSON строки
     * @param json JSON строка
     * @param key ключ
     * @return значение или пустая строка
     */
    std::string extractJsonValue(const std::string& json, const std::string& key);
};

#endif