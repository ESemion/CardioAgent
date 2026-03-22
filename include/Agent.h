#ifndef AGENT_H
#define AGENT_H

//Макрос для поддержки OpenSSL
#define CPPHTTPLIB_OPENSSL_SUPPORT 1

#include "Task.h"
#include "Config.h"
#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <httplib.h>

class Agent {
public:
    Agent(const Config& config);
    ~Agent();
    
    bool registerAgent();
    void start(std::function<ExecutionResult(const Task&)> callback);
    void stop();
    bool uploadResults(const std::string& sessionId, const ExecutionResult& result);
    
private:
    Config m_config;
    std::string m_accessCode;
    std::unique_ptr<httplib::SSLClient> m_httpClient;
    std::atomic<bool> m_running;
    std::thread m_pollThread;
    std::thread m_taskThread;
    
    std::queue<std::pair<Task, std::function<ExecutionResult(const Task&)>>> m_taskQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
    std::atomic<bool> m_taskRunning;
    
    int m_consecutiveFailures;
    std::chrono::seconds m_currentPollInterval;
    
    std::string extractJsonValue(const std::string& json, const std::string& key);
    void pollingLoop(std::function<ExecutionResult(const Task&)> callback);
    void taskWorker();
    void increasePollInterval();
    void resetPollInterval();
};

#endif
