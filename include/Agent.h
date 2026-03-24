#ifndef AGENT_H
#define AGENT_H

#include "Task.h"
#include "Config.h"
#include <string>
#include <memory>
#include <functional>
#include <queue>
#include "ServerClient.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

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
    explicit Agent(const Config& config);
    ~Agent();

    /**
     * @brief Запуск двух рабочих потоков: опрос сервера и выполнение заданий.
     *        Включает проверку доступности сервера и регистрацию перед запуском.
     * @param callback функция, которая будет вызвана для каждого полученного задания
     */
    void start(std::function<ExecutionResult(const Task&)> callback);

    /**
     * @brief Корректно останавливает все потоки.
     */
    void stop();

private:
    Config m_config;                           ///< настройки агента
    ServerClient m_server;                     ///< HTTP клиент
    std::atomic<bool> m_running{false};        ///< флаг работы
    std::atomic<int> m_currentPollInterval;    ///< текущий интервал опроса (с учётом backoff)

    std::thread m_pollThread;                  ///< поток опроса
    std::thread m_taskThread;                  ///< поток выполнения задач

    // Очередь задач — связывает поток опроса с потоком выполнения
    std::queue<std::pair<Task, std::function<ExecutionResult(const Task&)>>> m_taskQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
};

#endif
