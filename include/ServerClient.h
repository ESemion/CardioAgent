#ifndef SERVERCLIENT_H
#define SERVERCLIENT_H

//Макрос для поддержки OpenSSL
#define CPPHTTPLIB_OPENSSL_SUPPORT 1

#include <string>
#include <memory>
#include <vector>
#include <httplib.h>
#include "Task.h"


/**
 * @brief Низкоуровневый HTTP-клиент для взаимодействия с API сервера
 */
class ServerClient {
public:
    /**
     * @param url Полный адрес сервера (например, https://example.com:443)
     */
    explicit ServerClient(const std::string& url);
    
    /**
     * @note Деструктор определен в .cpp из-за std::unique_ptr и forward declaration
     */
    ~ServerClient();

    /**
     * @brief Проверка доступности сервера (GET /)
     * @return true если сервер отвечает
     */
    bool checkAvailability() const;

    /**
     * @brief Регистрация нового агента в системе
     * @return true если регистрация успешна и получен access_code
     */
    bool registerAgent(const std::string& uid, const std::string& descr, std::string& outAccessCode);

    /**
     * @brief Проверка наличия новых задач на сервере
     * @return true если задача успешно получена и распарсена
     */
    bool pollTask(const std::string& uid, const std::string& descr, const std::string& accessCode, Task& outTask);

    /**
     * @brief Передача результатов выполнения задачи и файлов на сервер
     */
    bool uploadResults(const std::string& uid, const std::string& accessCode, const std::string& sessionId, const ExecutionResult& result);

private:
    std::unique_ptr<httplib::SSLClient> m_client;
};

#endif