#ifndef AGENT_H
#define AGENT_H

// ВАЖНО: Этот макрос должен быть определён ДО включения httplib.h
// Макрос для поддержки OPENSSL
#define CPPHTTPLIB_OPENSSL_SUPPORT 1
#include "Task.h"
#include "Config.h"
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <httplib.h>




/**
 * Главный класс агента
 * Управляет регистрацией и опросом сервера
 */
class Agent {
private:
    Config m_config;           // Настройки
    std::string m_accessCode;  // Код доступа (от сервера)
    
    std::unique_ptr<httplib::SSLClient> m_httpClient;  // HTTP клиент
    std::atomic<bool> m_running;     // Флаг работы
    std::thread m_pollThread;         // Поток опроса

public:
    Agent(const Config& config);
    ~Agent();
    
    /**
     * Регистрация на сервере
     * @return true если успешно
     */
    bool registerAgent();
    
    /**
     * Запуск цикла опроса
     * @param callback функция для обработки полученных заданий
     */
    void start(std::function<ExecutionResult(const Task&)> callback);
    
    /**
     * Остановка опроса
     */
    void stop();

    /**
     * Отправляет результаты выполнения на сервер
     * @param sessionId ID сессии задания
     * @param success успешно ли выполнено
     * @param message сообщение о результате
     * @param files список файлов для отправки
     * @return true если отправка успешна
     */
    bool uploadResults(const std::string& sessionId, const ExecutionResult& result);

    bool confirmTaskReceived(const std::string& sessionId);
};

#endif // AGENT_H