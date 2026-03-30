#include <iostream>
#include <csignal>
#include <thread>
#include <atomic>
#include <sys/stat.h>

#include "Config.h"
#include "Agent.h"
#include "TaskExecutor.h"
#include "Logger.h"

// Атомарный флаг для управления циклом ожидания
std::atomic<bool> g_running(true);
Agent* g_agent_ptr = nullptr;

// Обработчик сигналов для корректного завершения
void signalHandler(int signal) {
    g_running = false;
    if (g_agent_ptr) {
        g_agent_ptr->stop();
    }
}

int main(int argc, char* argv[]) {
    // Создаем необходимые директории
#ifdef _WIN32
    _mkdir("logs");
    _mkdir("temp");
#else
    mkdir("logs", 0755);
    mkdir("temp", 0755);
#endif

    // Настройка логгера
    Logger::instance().setLevel(LogLevel::INFO);
    Logger::instance().setLogFile("logs/agent.log");
    Logger::instance().info("CardioAgent v1.0 запущен");

    // Определение пути к конфигу
    std::string configPath = "config/agent.ini";
    if (argc > 1) {
        configPath = argv[1];
    }

    Config config;
    if (!config.load(configPath)) {
        Logger::instance().error("Не удалось загрузить конфигурацию: " + configPath);
        return 1;
    }

    // Создаем объекты управления
    Agent agent(config);
    g_agent_ptr = &agent;
    TaskExecutor executor;

    // Регистрация обработчиков сигналов
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Запускаем агента. 
    // Регистрация на сервере и запуск потоков теперь инкапсулированы в start()
    agent.start([&executor](const Task& task) -> ExecutionResult {
        return executor.execute(task);
    });

    Logger::instance().info("Агент запущен. Нажмите Ctrl+C для остановки.");

    // Основной поток просто "спит", пока работают фоновые потоки агента
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    Logger::instance().info("Завершение работы программы...");
    return 0;
}