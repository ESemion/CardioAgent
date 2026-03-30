#ifndef TASKEXECUTOR_H
#define TASKEXECUTOR_H

#include "Task.h"

/**
 * @class TaskExecutor
 * @brief Исполнитель заданий
 * 
 * Выполняет полученные от сервера задания.
 */
class TaskExecutor {
public:
    TaskExecutor();
    
    /**
     * @brief Выполнить задание
     * @param task задание от сервера
     * @return результат выполнения
     */
    ExecutionResult execute(const Task& task);
};

#endif