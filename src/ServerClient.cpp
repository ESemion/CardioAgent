#include "ServerClient.h"
#include "Logger.h"

#include <fstream>
#include <sstream>
#include <cstdio>

namespace {
    // Внутренняя функция для безопасной вставки строк в JSON-тело
    std::string escapeJson(const std::string& s) {
        std::ostringstream o;
        for (auto c : s) {
            switch (c) {
                case '"':  o << "\\\""; break;
                case '\\': o << "\\\\"; break;
                case '\b': o << "\\b";  break;
                case '\f': o << "\\f";  break;
                case '\n': o << "\\n";  break;
                case '\r': o << "\\r";  break;
                case '\t': o << "\\t";  break;
                default:   o << c;      break;
            }
        }
        return o.str();
    }

    // Простой парсер строковых значений из JSON-ответа
    std::string extractJsonValue(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\":\"";
        size_t start = json.find(search);
        if (start == std::string::npos) return "";
        start += search.length();
        size_t end = json.find("\"", start);
        return (end == std::string::npos) ? "" : json.substr(start, end - start);
    }

    // Извлечение хоста и порта из URL для инициализации SSLClient
    void parseUrl(const std::string& url, std::string& host, int& port) {
        size_t protocolEnd = url.find("://");
        if (protocolEnd == std::string::npos) return;

        size_t hostStart = protocolEnd + 3;
        size_t pathStart = url.find("/", hostStart);
        std::string hostPort = url.substr(hostStart, pathStart - hostStart);

        size_t colonPos = hostPort.find(':');
        if (colonPos != std::string::npos) {
            host = hostPort.substr(0, colonPos);
            port = std::stoi(hostPort.substr(colonPos + 1));
        } else {
            host = hostPort;
            port = 443;
        }
    }
}

ServerClient::ServerClient(const std::string& url) {
    std::string host;
    int port;
    parseUrl(url, host, port);

    m_client = std::make_unique<httplib::SSLClient>(host, port);
    m_client->set_connection_timeout(5);
    m_client->set_read_timeout(10);
}

ServerClient::~ServerClient() = default;

bool ServerClient::checkAvailability() const {
    auto res = m_client->Get("/");
    return res != nullptr;
}

bool ServerClient::registerAgent(const std::string& uid, const std::string& descr, std::string& outAccessCode) {
    std::string body = "{\"UID\":\"" + escapeJson(uid) + 
                       "\",\"descr\":\"" + escapeJson(descr) + "\"}";
    
    auto res = m_client->Post("/app/webagent1/api/wa_reg/", body, "application/json");
    if (!res || res->status != 200) return false;

    if (extractJsonValue(res->body, "code_responce") == "-3") return false;
    
    outAccessCode = extractJsonValue(res->body, "access_code");
    return !outAccessCode.empty();
}

bool ServerClient::pollTask(const std::string& uid, const std::string& descr, const std::string& accessCode, Task& outTask) {
    std::string body = "{\"UID\":\"" + escapeJson(uid) +
                      "\",\"descr\":\"" + escapeJson(descr) +
                      "\",\"access_code\":\"" + escapeJson(accessCode) + "\"}";

    auto res = m_client->Post("/app/webagent1/api/wa_task/", body, "application/json");
    if (!res || res->status != 200){ return false;}

    std::string code = extractJsonValue(res->body, "code_responce");
    

    if (code == "1") {
        outTask.sessionId = extractJsonValue(res->body, "session_id");
        outTask.taskCode = extractJsonValue(res->body, "task_code");
        outTask.options = extractJsonValue(res->body, "options");
        outTask.status = extractJsonValue(res->body, "status");
        return true;
    }
    else if (code == "0") {
        outTask.status = extractJsonValue(res->body, "status");
        return true;
    }
    else {
    
        return false;
    }
}

bool ServerClient::uploadResults(const std::string& uid, const std::string& accessCode, const std::string& sessionId, const ExecutionResult& result) {
    std::string resultJson = "{\"UID\":\"" + escapeJson(uid) +
                             "\",\"access_code\":\"" + escapeJson(accessCode) +
                             "\",\"message\":\"" + escapeJson(result.message) +
                             "\",\"files\":" + std::to_string(result.files.size()) +
                             ",\"session_id\":\"" + sessionId + "\"}";

    httplib::UploadFormDataItems items;
    items.push_back({"result_code", std::to_string(result.success ? 0 : -1)});
    items.push_back({"result", resultJson});

    int fileIdx = 1;
    for (const auto& path : result.files) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            size_t pos = path.find_last_of("/\\");
            std::string name = (pos != std::string::npos) ? path.substr(pos + 1) : path;
            items.push_back({"file" + std::to_string(fileIdx++), content, name, "application/octet-stream"});
        }
    }

    auto res = m_client->Post("/app/webagent1/api/wa_result/", items);

    if (!res || res->status != 200 || extractJsonValue(res->body, "code_responce") != "0") {
        return false;
    }

    // Удаляем временные файлы после успешной отправки
    for (const auto& path : result.files) {
        if (std::remove(path.c_str()) == 0) {
            Logger::instance().debug("Удалён временный файл: " + path);
        } else {
            Logger::instance().warning("Не удалось удалить временный файл: " + path);
        }
    }

    return true;
}
