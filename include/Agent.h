#ifndef AGENT_H
#define AGENT_H

#define CPPHTTPLIB_OPENSSL_SUPPORT 1

#include "Task.h"
#include "Config.h"
#include <string>
#include <memory>
#include <functional>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <httplib.h>

/**
 * @class Agent
 * @brief Управляет регистрацией и опросом сервера
 *
 * Архитектура: два потока — опрос (pollThread) кладёт задания в очередь,
 * выполнение (taskThread) забирает и обрабатывает. Это позволяет не блокировать
 * опрос сервера пока задание выполняется.
 */
class Agent {
public:
    Agent(const Config& config);
    ~Agent();

    /// @brief Проверка связи с сервером (GET /) перед началом работы
    bool checkServerAvailability();

    /**
     * @brief Одноразовая регистрация агента на сервере
     *
     * Если access_code уже сохранён в конфиге — регистрация пропускается,
     * т.к. сервер не выдаёт код повторно.
     */
    bool registerAgent();

    /**
     * @brief Запуск двух рабочих потоков: опрос сервера и выполнение заданий
     * @param callback функция, которая будет вызвана для каждого полученного задания
     */
    void start(std::function<ExecutionResult(const Task&)> callback);

    void stop();

    /**
     * @brief Отправка результатов выполнения на сервер (multipart/form-data)
     *
     * При успешной отправке удаляет временные файлы из result.files.
     */
    bool uploadResults(const std::string& sessionId, const ExecutionResult& result);

private:
    Config m_config;
    std::unique_ptr<httplib::SSLClient> m_httpClient;
    std::atomic<bool> m_running;

    std::thread m_pollThread;
    std::thread m_taskThread;

    // Очередь задач — связывает поток опроса с потоком выполнения
    std::queue<std::pair<Task, std::function<ExecutionResult(const Task&)>>> m_taskQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
    std::atomic<bool> m_taskRunning;
    std::atomic<int> m_currentPollInterval;

    std::string extractJsonValue(const std::string& json, const std::string& key);
};

#endif
