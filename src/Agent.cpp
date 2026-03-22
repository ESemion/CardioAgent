#include "Agent.h"
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

Agent::Agent(const Config& config) 
    : m_config(config)
    , m_running(false)
    , m_taskRunning(false)
    , m_consecutiveFailures(0)
    , m_currentPollInterval(std::chrono::seconds(config.getPollInterval())) {
    
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
    m_httpClient->set_connection_timeout(5);
    m_httpClient->set_read_timeout(10);
    
    Logger::instance().debug("HTTP клиент создан: " + host + ":" + std::to_string(port));
    m_accessCode = m_config.getAccessCode();
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

void Agent::increasePollInterval() {
    m_consecutiveFailures++;
    int baseInterval = m_config.getPollInterval();
    int newInterval = baseInterval * (1 << m_consecutiveFailures);
    
    if (newInterval > 300) newInterval = 300;
    m_currentPollInterval = std::chrono::seconds(newInterval);
    
    Logger::instance().warning(std::to_string(m_consecutiveFailures) + 
                               " последовательных сбоев. Увеличиваем интервал до " + 
                               std::to_string(newInterval) + " секунд.");
}

void Agent::resetPollInterval() {
    m_consecutiveFailures = 0;
    m_currentPollInterval = std::chrono::seconds(m_config.getPollInterval());
}

bool Agent::registerAgent() {
    if (!m_config.getAccessCode().empty()) {
        Logger::instance().info("Регистрация отменена, код доступа уже существует");
        m_accessCode = m_config.getAccessCode();
        return true;
    }

    Logger::instance().info("Регистрация агента на сервере");
    
    std::string body = "{\"UID\":\"" + m_config.getUid() + 
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

    m_accessCode = accessCode;
    m_config.setAccessCode(accessCode);
    m_config.save("config/agent.ini");

    Logger::instance().info("Регистрация успешна");
    return true;
}

void Agent::start(std::function<ExecutionResult(const Task&)> callback) {
    m_running = true;
    m_taskRunning = true;
    
    m_pollThread = std::thread(&Agent::pollingLoop, this, callback);
    m_taskThread = std::thread(&Agent::taskWorker, this);
    
    Logger::instance().info("Агент запущен. Ожидание команд...");
}

void Agent::stop() {
    m_running = false;
    m_taskRunning = false;
    m_queueCV.notify_all();
    
    if (m_pollThread.joinable()) m_pollThread.join();
    if (m_taskThread.joinable()) m_taskThread.join();
    
    Logger::instance().info("Агент остановлен");
}

void Agent::pollingLoop(std::function<ExecutionResult(const Task&)> callback) {
    Logger::instance().info("Запущен цикл опроса. Начальный интервал: " + 
                            std::to_string(m_currentPollInterval.count()) + " сек");
    
    while (m_running) {
        std::string body = "{\"UID\":\"" + m_config.getUid() +
                          "\",\"descr\":\"" + m_config.getDescription() +
                          "\",\"access_code\":\"" + m_accessCode + "\"}";
        
        auto res = m_httpClient->Post("/app/webagent1/api/wa_task/", body, "application/json");
        
        if (!res) {
            Logger::instance().error("Не удалось подключиться к серверу");
            increasePollInterval();
        } 
        else if (res->status != 200) {
            Logger::instance().error("HTTP " + std::to_string(res->status));
            increasePollInterval();
        }
        else {
            resetPollInterval();
            
            std::string code = extractJsonValue(res->body, "code_responce");
            std::string msg = extractJsonValue(res->body, "msg");
            
            if (code == "1") {
                Task task;
                task.sessionId = extractJsonValue(res->body, "session_id");
                task.taskCode = extractJsonValue(res->body, "task_code");
                task.options = extractJsonValue(res->body, "options");
                
                Logger::instance().info("Получено задание: " + task.taskCode);
                
                {
                    std::lock_guard<std::mutex> lock(m_queueMutex);
                    m_taskQueue.push({task, callback});
                }
                m_queueCV.notify_one();
            }
            else if (code != "0") {
                Logger::instance().error("Возникла ошибка: " + msg);
            }
        }
        
        if (m_running) {
            std::this_thread::sleep_for(m_currentPollInterval);
        }
    }
    
    Logger::instance().info("Цикл опроса остановлен");
}

void Agent::taskWorker() {
    while (m_taskRunning) {
        std::pair<Task, std::function<ExecutionResult(const Task&)>> taskItem;
        
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(lock, [this] {
                return !m_taskQueue.empty() || !m_taskRunning;
            });
            
            if (!m_taskRunning) break;
            
            taskItem = m_taskQueue.front();
            m_taskQueue.pop();
        }
        
        Logger::instance().info("Выполнение задания: " + taskItem.first.taskCode);
        ExecutionResult result = taskItem.second(taskItem.first);
        uploadResults(taskItem.first.sessionId, result);
    }
    
    Logger::instance().info("Поток выполнения задач остановлен");
}

bool Agent::uploadResults(const std::string& sessionId, const ExecutionResult& result) {
    Logger::instance().info("Отправка результатов для сессии: " + sessionId);
    
    int resultCode = result.success ? 0 : -1;
    
    std::string resultJson = "{\"UID\":\"" + m_config.getUid() +
                             "\",\"access_code\":\"" + m_accessCode +
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
