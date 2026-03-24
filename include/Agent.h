#ifndef AGENT_H
#define AGENT_H


#include "Task.h"
#include "Config.h"
#include <string>
#include <memory>
#include <functional>
#include <queue>
#include "ServerClient.h"


// Библиотеки для работы с потоками
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

/**
 * @brief Высокоуровневый оркестратор работы агента.
 * Управляет очередью задач и многопоточностью.
 */
class Agent {
public:
    explicit Agent(const Config& config);
    ~Agent();

    /**
     * @brief Запускает потоки опроса сервера и выполнения задач.
     * @param callback Функция, которая будет выполнять саму работу (бизнес-логику).
     */
    void start(std::function<ExecutionResult(const Task&)> callback);

    /**
     * @brief Корректно останавливает все потоки.
     */
    void stop();

private:
    Config m_config;                           ///< настройки агента
    ServerClient m_server;                     ///< HTTP клиент
    std::atomic<bool> m_running{false};     ///< флаг работы
    std::atomic<int> m_currentPollInterval;    ///< текущий интервал опроса (с учётом backoff)

    std::thread m_pollThread;                  ///< поток опроса
    std::thread m_taskThread;                  ///< поток выполнения задач

    // Очередь хранит пару: данные задачи и функцию-обработчик
    std::queue<std::pair<Task, std::function<ExecutionResult(const Task&)>>> m_taskQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
};

#endif