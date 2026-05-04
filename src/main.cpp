#ifdef _WIN32
#include <direct.h>
#define mkdir(dir) _mkdir(dir)
#endif

#include <iostream>
#include <csignal>
#include <thread>
#include <atomic>
#include <filesystem>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

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

static std::string getExeDir() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
#elif defined(__APPLE__)
    char path[1024];
    uint32_t size = sizeof(path);
    _NSGetExecutablePath(path, &size);
#else
    char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    path[len > 0 ? len : 0] = '\0';
#endif
    return std::filesystem::path(path).parent_path().string();
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

    // Определение пути к конфигу:
    // 1. agent.ini рядом с исполняемым файлом (релиз)
    // 2. config/agent.ini (дев-сборка)
    std::string configPath;
    if (argc > 1) {
        configPath = argv[1];
    } else {
        std::string exePath = getExeDir() + "/agent.ini";
        configPath = std::filesystem::exists(exePath) ? exePath : "config/agent.ini";
    }

    Config config;
    if (!config.load(configPath)) {
        Logger::instance().error("Не удалось загрузить конфигурацию: " + configPath);
        return 1;
    }

    // Создаем объекты управления
    Agent agent(config);
    g_agent_ptr = &agent;
    TaskExecutor executor(config);

    // Регистрация обработчиков сигналов
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Запускаем агента. 
    // Регистрация на сервере и запуск потоков теперь инкапсулированы в start()
    if (!agent.start([&executor](const Task& task) -> ExecutionResult {
        return executor.execute(task);
    })) {
        Logger::instance().error("Не удалось запустить агент (ошибка связи или регистрации)");
        return 1; // Выходим из программы с кодом ошибки
    }

    Logger::instance().info("Агент запущен. Нажмите Ctrl+C для остановки.");

    // Основной поток просто "спит", пока работают фоновые потоки агента
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    Logger::instance().info("Завершение работы программы...");
    return 0;
}