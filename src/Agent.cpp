#include "Agent.h"

#include <iostream>

// Разбирает URL на хост и порт
// Пример: "https://server.ru:8080" -> host="server.ru", port=8080
static bool parseUrl(const std::string& url, std::string& host, int& port) {
    // Ищем разделитель протокола "://"
    size_t protocolEnd = url.find("://");
    if (protocolEnd == std::string::npos) return false;
    
    // Начало хоста после "://"
    size_t hostStart = protocolEnd + 3;
    size_t pathStart = url.find("/", hostStart);
    
    // Извлекаем часть "хост:порт"
    std::string hostPort = url.substr(hostStart, pathStart - hostStart);
    size_t colonPos = hostPort.find(':');
    
    if (colonPos != std::string::npos) {
        host = hostPort.substr(0, colonPos);
        port = std::stoi(hostPort.substr(colonPos + 1));
    } else {
        host = hostPort;
        port = 443;  // стандартный порт HTTPS
    }
    return true;
}

// Простой парсинг JSON для извлечения значения по ключу
// Пример: extractFromJson('{"code":"0"}', "code") -> "0"
static std::string extractFromJson(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":\"";
    size_t start = json.find(search);
    if (start == std::string::npos) return "";
    
    start += search.length();
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "";
    
    return json.substr(start, end - start);
}

Agent::Agent(const Config& config) : m_config(config), m_running(false) {
    std::string host;
    int port;
    
    if (parseUrl(config.getServerUrl(), host, port)) {
        // Создаём SSL клиент для защищённого соединения
        m_httpClient = std::make_unique<httplib::SSLClient>(host, port);
        
        // Отключаем проверку SSL (для тестового сервера с самоподписанным сертификатом)
        m_httpClient->enable_server_certificate_verification(false);
        
        // Таймаут подключения 5 секунд
        m_httpClient->set_connection_timeout(5);

    }
}

Agent::~Agent() {
    stop();
}

bool Agent::registerAgent() {
    std::cout << "\n--- Регистрация на сервере ---" << std::endl;
    
    // Формируем JSON с данными агента
    std::string body = "{\"UID\":\"" + m_config.getUid() + 
                      "\",\"descr\":\"" + m_config.getDescription() + "\"}";
    
    std::cout << "Отправляем: " << body << std::endl;
    
    // POST запрос на регистрацию
    auto res = m_httpClient->Post("/app/webagent1/api/wa_reg/", body, "application/json");
    
    if (!res) {
        std::cerr << "Ошибка: сервер не отвечает" << std::endl;
        return false;
    }
    
    std::cout << "Ответ: " << res->body << std::endl;
    
    // Извлекаем access_code из ответа
    m_accessCode = extractFromJson(res->body, "access_code");
    
    if (m_accessCode.empty()) {
        std::cerr << "Ошибка регистрации" << std::endl;
        return false;
    }
    
    std::cout << "Регистрация успешна, access_code: " << m_accessCode << std::endl;
    return true;
}

void Agent::start(std::function<ExecutionResult(const Task&)> callback) {
    m_running = true;
    
    m_pollThread = std::thread([this, callback]() {
        std::cout << "\nЗапуск опроса (интервал " << m_config.getPollInterval() << " сек)" << std::endl;
        
        while (m_running) {
            // Формируем запрос с access_code
            std::string body = "{\"UID\":\"" + m_config.getUid() + 
                              "\",\"descr\":\"" + m_config.getDescription() + 
                              "\",\"access_code\":\"" + m_accessCode + "\"}";
            
            // Спрашиваем сервер, есть ли задание
            auto res = m_httpClient->Post("/app/webagent1/api/wa_task/", body, "application/json");
            
            if (res && res->status == 200) {
                std::string code = extractFromJson(res->body, "code_responce");
                
                // code="1" означает, что есть задание
                if (code == "1") {
                    Task task;
                    task.sessionId = extractFromJson(res->body, "session_id");
                    task.taskCode = extractFromJson(res->body, "task_code");
                    task.options = extractFromJson(res->body, "options");
                    
                    std::cout << "\nПолучено задание: " << task.taskCode << std::endl;

                    ExecutionResult result = callback(task); // Вызываем обработчик
                    uploadResults(task.sessionId, result);
                    
                }
                else if (code == "0") {
                    // НЕТ ЗАДАНИЙ
                    std::cout << "\nНет заданий (WAIT). Сервер говорит ждать." << std::endl;
                    
                    // Можем показать статус из ответа
                    std::string status = extractFromJson(res->body, "status");
                    if (!status.empty()) {
                        std::cout << "   Статус: " << status << std::endl;
                    }
                }
                else if (code == "-2") {
                    // НЕВЕРНЫЙ КОД ДОСТУПА
                    std::cerr << "\nОШИБКА: Неверный код доступа! Нужна перерегистрация." << std::endl;
                    
                    // Пробуем перерегистрироваться автоматически
                    std::cout << "   Пытаемся перерегистрироваться..." << std::endl;
                    if (registerAgent()) {
                        std::cout << "   Перерегистрация успешна, продолжаем работу" << std::endl;
                    } else {
                        std::cerr << "   Перерегистрация не удалась" << std::endl;
                    }
                }
                else if (code == "-3") {
                    // АГЕНТ НЕ ЗАРЕГИСТРИРОВАН
                    std::cerr << "\nОШИБКА: Агент не зарегистрирован!" << std::endl;
                    
                    // Пробуем зарегистрироваться
                    std::cout << "   Пытаемся зарегистрироваться..." << std::endl;
                    if (registerAgent()) {
                        std::cout << "   Регистрация успешна, продолжаем работу" << std::endl;
                    } else {
                        std::cerr << "   Регистрация не удалась" << std::endl;
                    }
                }
                else {
                    // НЕИЗВЕСТНЫЙ КОД ОШИБКИ
                    std::cerr << "\nНеизвестный код ответа: " << code << std::endl;
                    std::cerr << "   Полный ответ: " << res->body << std::endl;
                }
            }
            
            // Ждём следующий опрос (проверяем флаг каждую секунду для быстрой остановки)
            for (int i = 0; i < m_config.getPollInterval() && m_running; i++) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    });
}

void Agent::stop() {
    m_running = false;
    if (m_pollThread.joinable()) {
        m_pollThread.join();
    }
}


bool Agent::uploadResults(const std::string& sessionId, const ExecutionResult& result) {
    std::cout << "\n--- Отправка результатов на сервер ---" << std::endl;
    
    int resultCode = result.success ? 0 : -1;
    
    // Используем поля из result
    std::string resultJson = "{\"UID\":\"" + m_config.getUid() + 
                            "\",\"access_code\":\"" + m_accessCode + 
                            "\",\"message\":\"" + result.message + 
                            "\",\"files\":" + std::to_string(result.files.size()) + 
                            ",\"session_id\":\"" + sessionId + "\"}";
    
    // СОЗДАЁМ ВЕКТОР ДАННЫХ (тип уже определён в httplib.h)
    httplib::UploadFormDataItems items;
    
    // 1. Добавляем result_code
    httplib::UploadFormData resultCodeItem;
    resultCodeItem.name = "result_code";
    resultCodeItem.content = std::to_string(resultCode);
    items.push_back(resultCodeItem);
    
    // 2. Добавляем result (JSON строка)
    httplib::UploadFormData resultItem;
    resultItem.name = "result";
    resultItem.content = resultJson;
    items.push_back(resultItem);
    
    // 3. Добавляем файлы
    int fileIndex = 1;
    for (const auto& filePath : result.files) {
        // Открываем файл
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Не удалось открыть файл: " << filePath << std::endl;
            continue;
        }
        
        // Читаем содержимое
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();
        
        // Получаем имя файла
        size_t pos = filePath.find_last_of("/\\");
        std::string fileName = (pos != std::string::npos) ? 
                                filePath.substr(pos + 1) : filePath;
        
        // Создаём элемент для файла
        httplib::UploadFormData fileItem;
        fileItem.name = "file" + std::to_string(fileIndex);
        fileItem.content = content;
        fileItem.filename = fileName;
        fileItem.content_type = "application/octet-stream";
        
        items.push_back(fileItem);
        
        std::cout << "  Файл " << fileIndex << ": " << fileName 
                  << " (" << content.size() << " байт)" << std::endl;
        
        fileIndex++;
    }
    
    // ОТПРАВЛЯЕМ
    std::cout << "Отправка " << items.size() << " частей..." << std::endl;
    
    // Используем метод Post, который принимает UploadFormDataItems
    auto res = m_httpClient->Post("/app/webagent1/api/wa_result/", items);
    
    if (!res) {
        std::cerr << "Ошибка: сервер не отвечает" << std::endl;
        return false;
    }
    
    if (res->status != 200) {
        std::cerr << "Ошибка HTTP: " << res->status << std::endl;
        return false;
    }
    
    std::cout << "Ответ сервера: " << res->body << std::endl;
    
    // Проверяем ответ
    std::string code = extractFromJson(res->body, "code_responce");
    std::string msg = extractFromJson(res->body, "msg");

    std::cout << "Код ответа: '" << code << "', сообщение: '" << msg << "'" << std::endl;

    if (code == "0") {
        std::cout << "✅ Результаты успешно отправлены!" << std::endl;
        return true;
    } else if (code.empty()) {
        std::cerr << "❌ Не удалось распарсить ответ сервера" << std::endl;
        return false;
    } else {
        std::cerr << "❌ Ошибка отправки: " << msg << std::endl;
        return false;
    }
}