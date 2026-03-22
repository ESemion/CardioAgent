#include "TaskExecutor.h"
#include "Logger.h"
#include <fstream>
#include <thread>
#include <chrono>

TaskExecutor::TaskExecutor() {
    Logger::instance().debug("TaskExecutor создан");
}

ExecutionResult TaskExecutor::execute(const Task& task) {
    Logger::instance().info("Выполнение задания: " + task.taskCode + " (сессия: " + task.sessionId + ")");
    
    ExecutionResult result;
    result.success = true;
    result.message = "Задание выполнено успешно";
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::string filename = "temp/result_" + task.sessionId + ".txt";
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "Session: " << task.sessionId << std::endl;
        file << "Task code: " << task.taskCode << std::endl;
        file << "Options: " << task.options << std::endl;
        file.close();
        result.files.push_back(filename);
        Logger::instance().debug("Создан файл: " + filename);
    } else {
        Logger::instance().warning("Не удалось создать файл: " + filename);
    }
    
    return result;
}