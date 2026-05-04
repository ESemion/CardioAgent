#include "Agent.h"
#include "Logger.h"

Agent::Agent(const Config& config)
    : m_config(config), m_server(config.getServerUrl()),
      m_running(false), m_currentPollInterval(config.getPollInterval()) {}

Agent::~Agent() {
    stop();
}

bool Agent::start(std::function<ExecutionResult(const Task&)> callback) {

    // Защита от повторного вызова
    if (m_running.exchange(true)) {
        Logger::instance().warning("Попытка повторного запуска Agent::start() проигнорирована.");
        return false;
    }

    // Проверяем доступность сервера перед регистрацией
    Logger::instance().info("Проверка доступности сервера...");
    if (!m_server.checkAvailability()) {
        Logger::instance().error("Сервер недоступен при запуске");
        m_running = false;
        return false;
    }
    Logger::instance().info("Сервер доступен");

    std::string accessCode = m_config.getAccessCode();
    if (accessCode=="") {
        // Если access_code уже получен — повторная регистрация не нужна,
        // сервер выдаёт код однократно при первом подключении
        if (!m_server.registerAgent(m_config.getUid(), m_config.getDescription(), accessCode)) {
            Logger::instance().info("Регистрация отменена");
            m_running = false;
            return false;
        }
        // Если получили новый код — сохраняем
        if (!accessCode.empty()) {
            m_config.setAccessCode(accessCode);
            m_config.save("config/agent.ini");
        }
    }

    // Поток опроса (Producer): кладёт задания в очередь
    m_currentPollInterval = m_config.getPollInterval();
    m_pollThread = std::thread([this, callback]() {
        Logger::instance().info("Запущен цикл опроса, интервал: " + std::to_string(m_config.getPollInterval()) + " сек");

        while (m_running) {
            Task task;
            if (m_server.pollTask(m_config.getUid(), m_config.getDescription(), m_config.getAccessCode(), task)) {
                // Сервер доступен — сбрасываем интервал к базовому
                if (m_currentPollInterval != m_config.getPollInterval()) {
                    Logger::instance().info("Сервер снова доступен, интервал опроса сброшен до " +
                                            std::to_string(m_config.getPollInterval()) + " сек");
                    m_currentPollInterval = m_config.getPollInterval();
                }

                if (task.status == "RUN") {
                    Logger::instance().info("Получена задача: " + task.taskCode);
                    // Добавляем задание в очередь, а не выполняем в потоке опроса
                    {
                        std::lock_guard<std::mutex> lock(m_queueMutex);
                        m_taskQueue.push({task, callback});
                    }
                    m_queueCV.notify_one();
                }
            } else {
                // Сервер недоступен — exponential backoff
                int newInterval = std::min(m_currentPollInterval.load() * 2, m_config.getMaxPollInterval());
                if (newInterval != m_currentPollInterval) {
                    m_currentPollInterval = newInterval;
                    Logger::instance().warning("Сервер недоступен, интервал опроса увеличен до " +
                                               std::to_string(newInterval) + " сек");
                } else {
                    Logger::instance().warning("Сервер недоступен, ожидание " +
                                               std::to_string(newInterval) + " сек");
                }
            }

            // Сон с возможностью прерывания при остановке агента
            for (int i = 0; i < m_currentPollInterval && m_running; i++) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        Logger::instance().info("Цикл опроса остановлен");
    });

    // Поток выполнения (Consumer): забирает задачи из очереди и исполняет
    m_taskThread = std::thread([this]() {
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

            Logger::instance().info("Выполнение задания: " + item.first.taskCode);
            ExecutionResult res = item.second(item.first);

            if (m_server.uploadResults(m_config.getUid(), m_config.getAccessCode(), item.first.sessionId, res)) {
                Logger::instance().info("Результат успешно отправлен");
            } else {
                Logger::instance().error("Ошибка при отправке результата");
            }
        }

        Logger::instance().info("Поток выполнения задач остановлен");
    });

    Logger::instance().info("Агент запущен. Ожидание команд...");
    return true;
}

void Agent::stop() {
    if (!m_running) return;

    m_running = false;
    m_queueCV.notify_all(); // Пробуждаем поток выполнения, чтобы он завершился

    if (m_pollThread.joinable()) m_pollThread.join();
    if (m_taskThread.joinable()) m_taskThread.join();

    Logger::instance().info("Агент остановлен");
}