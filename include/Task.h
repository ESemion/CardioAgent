#ifndef TASK_H
#define TASK_H

#include <string>
#include <vector>

/**
 * @struct Task
 * @brief Задание от сервера
 */
struct Task {
    std::string sessionId;  ///< идентификатор сессии
    std::string taskCode;   ///< код задания (CONF, UPLOAD, REBOOT)
    std::string options;    ///< параметры задания
};

/**
 * @struct ExecutionResult
 * @brief Результат выполнения задания
 */
struct ExecutionResult {
    bool success;                     ///< успешность выполнения
    std::vector<std::string> files;   ///< список созданных файлов
    std::string message;              ///< текстовое сообщение
};

#endif