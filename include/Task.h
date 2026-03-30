#ifndef TASK_H
#define TASK_H

#include <string>
#include <vector>

/**
 * @struct Task
 * @brief Задание, полученное от сервера
 */
struct Task {
    std::string sessionId;
    std::string taskCode;    ///< тип задания: CONF, UPLOAD, REBOOT и т.д.
    std::string options;
};

/**
 * @struct ExecutionResult
 * @brief Результат выполнения задания
 */
struct ExecutionResult {
    bool success;
    std::vector<std::string> files;  ///< пути к файлам в temp/, удаляются после отправки
    std::string message;
};

#endif // TASK_H
