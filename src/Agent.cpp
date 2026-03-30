#include "Agent.h"
#include "Logger.h"

Agent::Agent(const Config& config) : m_config(config), m_server(config.getServerUrl()) {}

Agent::~Agent() {
    stop();
}

void Agent::start(std::function<ExecutionResult(const Task&)> callback) {

    //Защита от повторного вызова
    if (m_running.exchange(true)) { 
        Logger::instance().warning("Попытка повторного запуска Agent::start() проигнорирована.");
        return;
    }

    std::string accessCode = m_config.getAccessCode();
    if (accessCode=="") {
        // Сначала пытаемся зарегистрироваться
        if (!m_server.registerAgent(m_config.getUid(), m_config.getDescription(), accessCode)) {
            Logger::instance().info("Регистрация отменена");
            return;
        }
        // Если получили новый код — сохраняем
        if (!accessCode.empty()) {
            m_config.setAccessCode(accessCode);
            m_config.save("config/agent.ini");
        }
        
    }
    
    m_running = true;

    // Поток опроса (Producer): запрашивает задачи и кладет в очередь
    m_pollThread = std::thread([this, callback]() {
        Logger::instance().info("Поток опроса запущен");
        
        while (m_running) {
            Task task;
            if (m_server.pollTask(m_config.getUid(), m_config.getDescription(), m_config.getAccessCode(), task)) {
                if (task.status == "RUN") {
                    Logger::instance().info("Получена задача: " + task.taskCode);
                    {
                        std::lock_guard<std::mutex> lock(m_queueMutex);
                        m_taskQueue.push({task, callback});
                    }
                    m_queueCV.notify_one();

            
                }
            }
            else {
                Logger::instance().error("Не удалось получить ответ от сервера");
            }
            // Сон с возможностью прерывания при остановке агента
            for (int i = 0; i < m_config.getPollInterval() && m_running; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    });

    // Поток выполнения (Consumer): забирает задачи из очереди и исполняет
    m_taskThread = std::thread([this]() {
        Logger::instance().info("Поток выполнения задач запущен");
        
        while (m_running) {
            std::pair<Task, std::function<ExecutionResult(const Task&)>> item;
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_queueCV.wait(lock, [this] { 
                    return !m_taskQueue.empty() || !m_running; 
                });
                
                if (!m_running && m_taskQueue.empty()) break;
                
                item = std::move(m_taskQueue.front());
                m_taskQueue.pop();
            }

            Logger::instance().info("Запуск выполнения: " + item.first.taskCode);
            ExecutionResult res = item.second(item.first);
            
            if (m_server.uploadResults(m_config.getUid(), m_config.getAccessCode(), item.first.sessionId, res)) {
                Logger::instance().info("Результат успешно отправлен");
            } else {
                Logger::instance().error("Ошибка при отправке результата");
            }
        }
    });
}

void Agent::stop() {
    if (!m_running) return;

    m_running = false;
    m_queueCV.notify_all(); // Пробуждаем поток выполнения, чтобы он завершился

    if (m_pollThread.joinable()) m_pollThread.join();
    if (m_taskThread.joinable()) m_taskThread.join();
    
    Logger::instance().info("Агент остановлен");
}