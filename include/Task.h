#ifndef TASK_H
#define TASK_H

#include <string>
#include <vector>

/**
 * @struct Task
 * @brief Единая структура задания
 */
struct Task {
    std::string sessionId;   ///< ID сессии от сервера
    std::string taskCode;    ///< CONF, UPLOAD, REBOOT
    std::string options;     ///< параметры
};


/**
 * @struct ExecutionResult
 * @brief Результат выполнения задания
 * 
 * Содержит информацию о том, как прошло выполнение:
 * - успех/ошибка
 * - какие файлы созданы
 * - текстовое сообщение
 */
struct ExecutionResult {
    bool success;                    ///< true = всё ок, false = ошибка
    std::vector<std::string> files;  ///< пути к созданным файлам
    std::string message;             ///< сообщение о результате
};

#endif // TASK_H