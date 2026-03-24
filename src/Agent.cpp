#include "Agent.h"
#include "Logger.h"


// В конструкторе реализована логика выделения из URL host и port для SSLClient
Agent::Agent(const Config& config) : m_config(config), m_running(false), m_currentPollInterval(config.getPollInterval()) {
    std::string url = m_config.getServerUrl();
    
    size_t protocolEnd = url.find("://");
    if (protocolEnd == std::string::npos) {
        Logger::instance().error("Некорректный URL сервера: " + url);
        return;
    }
    
    size_t hostStart = protocolEnd + 3;
    size_t pathStart = url.find("/", hostStart);
    std::string hostPort = url.substr(hostStart, pathStart - hostStart);
    
    std::string host;
    int port = 443;
    
    size_t colonPos = hostPort.find(':');
    if (colonPos != std::string::npos) {
        host = hostPort.substr(0, colonPos);
        port = std::stoi(hostPort.substr(colonPos + 1));
    } else {
        host = hostPort;
    }
    
    m_httpClient = std::make_unique<httplib::SSLClient>(host, port);

    // Проверка сертификата вроде как включена по умолчанию

    m_httpClient->set_connection_timeout(5);
    m_httpClient->set_read_timeout(10);
    
    Logger::instance().debug("HTTP клиент создан: " + host + ":" + std::to_string(port));
}


Agent::~Agent() {
    stop();
}


std::string Agent::extractJsonValue(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":\"";
    size_t start = json.find(search);
    if (start == std::string::npos) return "";
    
    start += search.length();
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "";
    
    return json.substr(start, end - start);
}



bool Agent::registerAgent() {

    // Если есть access_code, проверяем его
    if (!m_config.getAccessCode().empty()) {
        Logger::instance().info("Регистрация отменена, код доступа уже существует");
        return true;
    }

    Logger::instance().info("Регистрация агента на сервере");
    
    std::string body =  "{\"UID\":\"" + m_config.getUid() + 
                       "\",\"descr\":\"" + m_config.getDescription() + "\"}";
    
    auto res = m_httpClient->Post("/app/webagent1/api/wa_reg/", body, "application/json");
    
    if (!res) {
        Logger::instance().error("Ошибка регистрации: сервер не отвечает");
        return false;
    }

    auto accessCode = extractJsonValue(res->body, "access_code");
    auto codeResponce = extractJsonValue(res->body, "code_responce");
    
    
    if (codeResponce == "-3") {
        Logger::instance().error("Ошибка регистрации: агент зарегистрирован, но код доступа утерян");
        return false;
    }
    if (accessCode.empty()) {
        Logger::instance().error("Ошибка регистрации: некорректный ответ сервера");
        return false;
    }
    

    m_config.setAccessCode(accessCode);
    m_config.save("config/agent.ini");

    Logger::instance().info("Регистрация успешна");
    return true;
}



void Agent::start(std::function<ExecutionResult(const Task&)> callback) {
    m_running = true;
    m_taskRunning = true;
    
    // Поток для опроса сервера (основной)
    m_currentPollInterval = m_config.getPollInterval();
    m_pollThread = std::thread([this, callback]() {
        Logger::instance().info("Запущен цикл опроса, интервал: " + std::to_string(m_config.getPollInterval()) + " сек");

        while (m_running) {
            std::string body = "{\"UID\":\"" + m_config.getUid() +
                              "\",\"descr\":\"" + m_config.getDescription() +
                              "\",\"access_code\":\"" + m_config.getAccessCode() + "\"}";

            auto res = m_httpClient->Post("/app/webagent1/api/wa_task/", body, "application/json");

            if (res && res->status == 200) {
                // Сервер доступен — сбрасываем интервал к базовому
                if (m_currentPollInterval != m_config.getPollInterval()) {
                    Logger::instance().info("Сервер снова доступен, интервал опроса сброшен до " +
                                            std::to_string(m_config.getPollInterval()) + " сек");
                    m_currentPollInterval = m_config.getPollInterval();
                }

                std::string code = extractJsonValue(res->body, "code_responce");
                std::string msg = extractJsonValue(res->body, "msg");

                if (code == "1") {
                    Task task;
                    task.sessionId = extractJsonValue(res->body, "session_id");
                    task.taskCode = extractJsonValue(res->body, "task_code");
                    task.options = extractJsonValue(res->body, "options");

                    Logger::instance().info("Получено задание: " + task.taskCode);

                    // Вместо прямого выполнения - добавляем задание в очередь
                    {
                        std::lock_guard<std::mutex> lock(m_queueMutex);
                        m_taskQueue.push({task, callback});
                    }
                    m_queueCV.notify_one();  // Уведомляем поток выполнения
                }
                else if (code != "0") {
                    Logger::instance().error("Возникла ошибка: " + msg);
                }
            } else {
                // Сервер недоступен — увеличиваем интервал (exponential backoff)
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

            for (int i = 0; i < m_currentPollInterval && m_running; i++) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        Logger::instance().info("Цикл опроса остановлен");
    });
    
    // Поток для выполнения задач (новый)
    m_taskThread = std::thread([this]() {
        while (m_taskRunning) {
            std::pair<Task, std::function<ExecutionResult(const Task&)>> taskItem;
            
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                // Ожидаем появления задачи в очереди
                m_queueCV.wait(lock, [this] {
                    return !m_taskQueue.empty() || !m_taskRunning;
                });
                
                if (!m_taskRunning) break;
                
                taskItem = m_taskQueue.front();
                m_taskQueue.pop();
            }
            
            Logger::instance().info("Выполнение задания: " + taskItem.first.taskCode);
            
            // Выполняем задачу
            ExecutionResult result = taskItem.second(taskItem.first);
            
            // Отправляем результат
            uploadResults(taskItem.first.sessionId, result);
        }
        
        Logger::instance().info("Поток выполнения задач остановлен");
    });
    
    Logger::instance().info("Агент запущен. Ожидание команд...");
}



void Agent::stop() {
    m_running = false;
    m_taskRunning = false;
    
    // Пробуждаем поток, ожидающий очередь
    m_queueCV.notify_all();
    
    if (m_pollThread.joinable()) {
        m_pollThread.join();
    }
    if (m_taskThread.joinable()) {
        m_taskThread.join();
    }
    
    Logger::instance().info("Агент остановлен");
}



bool Agent::uploadResults(const std::string& sessionId, const ExecutionResult& result) {
    Logger::instance().info("Отправка результатов для сессии: " + sessionId);
    
    int resultCode = result.success ? 0 : -1;
    
    std::string resultJson = "{\"UID\":\"" + m_config.getUid() +
                             "\",\"access_code\":\"" +  m_config.getAccessCode() +
                             "\",\"message\":\"" + result.message +
                             "\",\"files\":" + std::to_string(result.files.size()) +
                             ",\"session_id\":\"" + sessionId + "\"}";
    
    httplib::UploadFormDataItems items;
    
    httplib::UploadFormData codeItem;
    codeItem.name = "result_code";
    codeItem.content = std::to_string(resultCode);
    items.push_back(codeItem);
    
    httplib::UploadFormData resultItem;
    resultItem.name = "result";
    resultItem.content = resultJson;
    items.push_back(resultItem);
    
    int fileIndex = 1;
    for (const auto& filePath : result.files) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            Logger::instance().warning("Не удалось открыть файл: " + filePath);
            continue;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();
        
        size_t pos = filePath.find_last_of("/\\");
        std::string fileName = (pos != std::string::npos) ? filePath.substr(pos + 1) : filePath;
        
        httplib::UploadFormData fileItem;
        fileItem.name = "file" + std::to_string(fileIndex);
        fileItem.content = content;
        fileItem.filename = fileName;
        fileItem.content_type = "application/octet-stream";
        items.push_back(fileItem);
        
        Logger::instance().debug("Добавлен файл: " + fileName);
        fileIndex++;
    }
    
    auto res = m_httpClient->Post("/app/webagent1/api/wa_result/", items);
    
    if (!res) {
        Logger::instance().error("Ошибка отправки: сервер не отвечает");
        return false;
    }
    
    if (res->status != 200) {
        Logger::instance().error("Ошибка отправки: HTTP " + std::to_string(res->status));
        return false;
    }
    
    std::string code = extractJsonValue(res->body, "code_responce");
    if (code == "0") {
        Logger::instance().info("Результаты успешно отправлены");
        return true;
    }
    
    Logger::instance().error("Ошибка отправки: " + extractJsonValue(res->body, "msg"));
    return false;
}