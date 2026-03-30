#include <iostream>
#include <csignal>
#include <thread>
#include <atomic>
#include <sys/stat.h>
#include "Config.h"
#include "Agent.h"
#include "TaskExecutor.h"
#include "Logger.h"

std::atomic<bool> g_running(true);
Agent* g_agent = nullptr;

void signalHandler(int signal) {
    g_running = false;
    if (g_agent) {
        g_agent->stop();
    }
}

int main(int argc, char* argv[]) {
    mkdir("logs", 0755);
    mkdir("temp", 0755);
    
    Logger::instance().setLevel(LogLevel::INFO);
    Logger::instance().setLogFile("logs/agent.log");
    Logger::instance().info("CardioAgent v1.0 запущен");
    
    std::string configPath = "config/agent.ini";
    if (argc > 1) {
        configPath = argv[1];
    }
    
    Config config;
    if (!config.load(configPath)) {
        Logger::instance().error("Не удалось загрузить конфигурацию");
        return 1;
    }
    
    Agent agent(config);
    g_agent = &agent;
    
    if (!agent.checkServerAvailability()) {
        Logger::instance().error("Сервер недоступен при запуске");
        return 1;
    }

    if (!agent.registerAgent()) {
        Logger::instance().error("Не удалось зарегистрироваться на сервере");
        return 1;
    }
    
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    TaskExecutor executor;
    
    agent.start([&executor](const Task& task) -> ExecutionResult {
        return executor.execute(task);
    });
    
    Logger::instance().info("Агент запущен. Нажмите Ctrl+C для остановки.");
    
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    Logger::instance().info("Агент остановлен");
    return 0;
}