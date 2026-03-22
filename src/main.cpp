#include <iostream>
#include <csignal>
#include <thread>
#include <atomic>
#include "Config.h"
#include "Agent.h"
#include "TaskExecutor.h"

std::atomic<bool> g_running(true);
Agent* g_agent = nullptr;

void signalHandler(int signal) {
    std::cout << "\n\n!!! Получен сигнал " << signal << " !!!" << std::endl;
    g_running = false;
    
    if (g_agent) {
        g_agent->stop();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "         CardioAgent v1.0              " << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::string configPath = "config/agent.ini";
    if (argc > 1) {
        configPath = argv[1];
        std::cout << "Путь к конфигу из аргументов: " << configPath << std::endl;
    } else {
        std::cout << "Путь к конфигу по умолчанию: " << configPath << std::endl;
    }
    
    // Загружаем конфигурацию
    Config config;
    if (!config.load(configPath)) {
        std::cerr << "Ошибка: не удалось загрузить конфигурацию" << std::endl;
        return 1;
    }
    
    // Создаём агента
    Agent agent(config);
    g_agent = &agent;
    
    // Регистрируемся на сервере
    if (!agent.registerAgent()) {
        std::cerr << "Ошибка: не удалось зарегистрироваться на сервере" << std::endl;
        return 1;
    }
    
    // Устанавливаем обработчик сигналов
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Создаём исполнитель заданий
    TaskExecutor executor;
    
    // Запускаем агента с колбэком для обработки заданий
    std::cout << "\nЗапуск агента... Нажмите Ctrl+C для остановки" << std::endl;
    
    agent.start([&executor](const Task& task) -> ExecutionResult {
        
        // ВЫПОЛНЯЕМ И ВОЗВРАЩАЕМ РЕЗУЛЬТАТ
        return executor.execute(task);  // ← возвращаем ExecutionResult
    });
    
    std::cout << "\n✅ Агент работает в фоновом режиме." << std::endl;
    std::cout << "Нажмите Ctrl+C для выхода.\n" << std::endl;
    
    // Ждём сигнала остановки
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "\n✅ Программа завершена." << std::endl;
    return 0;
}