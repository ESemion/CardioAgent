#include "TaskExecutor.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

TaskExecutor::TaskExecutor() {
    std::cout << "TaskExecutor создан" << std::endl;
}

ExecutionResult TaskExecutor::execute(const Task& task) {
    std::cout << "\n>>> ВЫПОЛНЯЮ ЗАДАНИЕ: " << task.taskCode << std::endl;
    std::cout << "    session: " << task.sessionId << std::endl;
    std::cout << "    options: " << task.options << std::endl;
    
    ExecutionResult result;
    result.success = true;
    result.message = "Задание выполнено (тест)";
    
    // Имитация работы - просто ждём 1 секунду
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Создаём тестовый файл
    std::string filename = "temp/result_" + task.sessionId + ".txt";
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "Тестовый результат для задания " << task.taskCode << std::endl;
        file << "Session: " << task.sessionId << std::endl;
        file << "Options: " << task.options << std::endl;
        file << "Время: " << std::time(nullptr) << std::endl;
        file.close();
        
        result.files.push_back(filename);
        std::cout << "    создан файл: " << filename << std::endl;
    }
    
    std::cout << "<<< ЗАДАНИЕ ВЫПОЛНЕНО" << std::endl;
    return result;
}