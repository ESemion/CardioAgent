#include "Agent.h"
#include "Logger.h"


// В конструкторе реализована логика выделения из URL host и port для SSLClient
Agent::Agent(const Config& config) : m_config(config), m_running(false) {
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
    Logger::instance().info("Регистрация агента на сервере");
    
    std::string body = "{\"UID\":\"" + m_config.getUid() + 
                       "\",\"descr\":\"" + m_config.getDescription() + "\"}";
    
    auto res = m_httpClient->Post("/app/webagent1/api/wa_reg/", body, "application/json");
    
    if (!res) {
        Logger::instance().error("Ошибка регистрации: сервер не отвечает");
        return false;
    }
    
    m_accessCode = extractJsonValue(res->body, "access_code");
    
    if (m_accessCode.empty()) {
        Logger::instance().error("Ошибка регистрации: некорректный ответ сервера");
        return false;
    }
    
    Logger::instance().info("Регистрация успешна");
    return true;
}


/* 
Пока опрос реализован плохо, надо реализовать разделение на поток опроса и потоком с очередью заданий и их выполнением
Это нужно чтобы даже если выполнение задания зависло, агент не завис и смог бы отправить команду по типу отмены

Нужно по максимуму сохранить текущую структуру кода. 

*/
void Agent::start(std::function<ExecutionResult(const Task&)> callback) {
    m_running = true;
    
    m_pollThread = std::thread([this, callback]() {
        Logger::instance().info("Запущен цикл опроса, интервал: " + std::to_string(m_config.getPollInterval()) + " сек");
        
        while (m_running) {
            std::string body = "{\"UID\":\"" + m_config.getUid() +
                              "\",\"descr\":\"" + m_config.getDescription() +
                              "\",\"access_code\":\"" + m_accessCode + "\"}";
            
            auto res = m_httpClient->Post("/app/webagent1/api/wa_task/", body, "application/json");
            
            if (res && res->status == 200) {
                std::string code = extractJsonValue(res->body, "code_responce");
                
                if (code == "1") {
                    Task task;
                    task.sessionId = extractJsonValue(res->body, "session_id");
                    task.taskCode = extractJsonValue(res->body, "task_code");
                    task.options = extractJsonValue(res->body, "options");
                    
                    Logger::instance().info("Получено задание: " + task.taskCode);
                    ExecutionResult result = callback(task);
                    uploadResults(task.sessionId, result);
                }
                else if (code == "-2" || code == "-3") {
                    Logger::instance().warning("Неверная регистрация, выполняем перерегистрацию...");
                    if (registerAgent()) {
                        Logger::instance().info("Перерегистрация успешна");
                    }
                }
                else if (code != "0") {
                    Logger::instance().warning("Неизвестный код ответа: " + code);
                }
            }
            
            for (int i = 0; i < m_config.getPollInterval() && m_running; i++) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        
        Logger::instance().info("Цикл опроса остановлен");
    });
}



void Agent::stop() {
    m_running = false;
    if (m_pollThread.joinable()) {
        m_pollThread.join();
    }
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