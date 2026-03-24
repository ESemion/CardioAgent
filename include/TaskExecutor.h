#ifndef TASKEXECUTOR_H
#define TASKEXECUTOR_H

#include "Task.h"

/**
 * @class TaskExecutor
 * @brief Исполнитель заданий, полученных от сервера
 */
class TaskExecutor {
public:
    TaskExecutor();

    /// @brief Выполнить задание и вернуть результат с путями к созданным файлам
    ExecutionResult execute(const Task& task);
};

#endif
