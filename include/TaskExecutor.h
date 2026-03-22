#ifndef TASKEXECUTOR_H
#define TASKEXECUTOR_H
#include "Task.h"
#include <string>
#include <vector>




/**
 * @class TaskExecutor
 * @brief Простейший исполнитель заданий
 * 
 * Временная реализация, которая просто имитирует работу
 * и создаёт тестовые файлы. В будущем будет заменён на
 * реальный запуск программ.
 */
class TaskExecutor {
public:

    TaskExecutor();
    
    /**
     * @brief Выполнить задание
     * 
     * @param task задание от сервера
     * @return ExecutionResult результат выполнения
     * 
     * Сейчас просто имитирует работу:
     * - ждёт 1 секунду
     * - создаёт тестовый файл в папке temp/
     * - возвращает успех
     */
    ExecutionResult execute(const Task& task);
};

#endif // TASKEXECUTOR_H