#ifndef TASKEXECUTOR_H
#define TASKEXECUTOR_H

#include "Task.h"
#include "Config.h"

/**
 * @class TaskExecutor
 * @brief Исполнитель заданий, полученных от сервера
 */
class TaskExecutor {
private:
    Config& m_config;

public:
    TaskExecutor(Config& config);

    /// @brief Выполнить задание и вернуть результат с путями к созданным файлам
    ExecutionResult execute(const Task& task);
};

#endif
