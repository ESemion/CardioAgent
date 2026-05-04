#include "TaskExecutor.h"
#include "Logger.h"
#include <fstream>
#include <thread>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

TaskExecutor::TaskExecutor(Config& config) : m_config(config) {
    Logger::instance().debug("TaskExecutor создан");
}

ExecutionResult TaskExecutor::execute(const Task& task) {
    Logger::instance().info("Выполнение задания: " + task.taskCode + " (сессия: " + task.sessionId + ")");

    ExecutionResult result;
    result.success = true;
    result.message = "Задание выполнено успешно";

    // --- ОБРАБОТКА КОМАНДЫ CONF ---
    if (task.taskCode == "CONF") {
        size_t eqPos = task.options.find('=');
        if (eqPos == std::string::npos) {
            result.success = false;
            result.message = "Ошибка: неверный формат (ожидалось ключ=значение)";
        } else {
            // Внутри TaskExecutor::execute в блоке if (task.taskCode == "CONF")

            std::string key = task.options.substr(0, eqPos);
            std::string value = task.options.substr(eqPos + 1);

            auto sanitizePath = [](std::string& s) {
                // 1. Убираем лишние пробелы и кавычки по краям
                s.erase(0, s.find_first_not_of(" \t\r\n\""));
                s.erase(s.find_last_not_of(" \t\r\n\"") + 1);

                // 2. Убираем экранирование слешей \/ -> /
                size_t pos = 0;
                while ((pos = s.find("\\/", pos)) != std::string::npos) {
                    s.replace(pos, 2, "/");
                    pos += 1;
                }
                
                // 3. Для Windows: если сервер прислал двойные обратные слеши \\ -> \
                pos = 0;
                while ((pos = s.find("\\\\", pos)) != std::string::npos) {
                    s.replace(pos, 2, "\\");
                    pos += 1;
                }
            };

            sanitizePath(key);
            sanitizePath(value);
            bool knownParam = true;
            if (key == "descr") {
                m_config.setDescription(value);
            } else if (key == "result_path") {
                m_config.setResultPath(value);
            } else if (key == "program_path") {
                m_config.setProgramPath(value);
            }
            else {
                knownParam = false;
                result.success = false;
                result.message = "Ошибка: неизвестный параметр '" + key + "'";
            }

            if (knownParam && !m_config.save("config/agent.ini")) {
                result.success = false;
                result.message = "Ошибка при сохранении конфига";
            }
        }
    }
    // --- ОБРАБОТКА КОМАНДЫ TIMEOUT ---
    else if (task.taskCode == "TIMEOUT") {
        try {
            m_config.setPollInterval(std::stoi(task.options));
            m_config.save("config/agent.ini");
        } catch (...) {
            result.success = false;
            result.message = "Неверный формат числа";
        }
    }
    // --- ОБРАБОТКА КОМАНДЫ TASK (ЗАПУСК ПРОГРАММЫ) ---
    else if (task.taskCode == "TASK") {
        std::string progPath = m_config.getProgramPath();
        
        if (progPath.empty() || !fs::exists(progPath)) {
            result.success = false;
            result.message = "Путь к программе не настроен или файл не найден: " + progPath;
        } else {
            // Формируем команду запуска
            std::string cmd;
            #ifdef _WIN32
                // Используем кавычки для путей с пробелами
                cmd = "\"" + progPath + "\" " + task.options;
            #elif __APPLE__
                // Для macOS: если это .app - через open, иначе напрямую
                if (progPath.find(".app") != std::string::npos) {
                    cmd = "open \"" + progPath + "\" --args " + task.options;
                } else {
                    cmd = "\"" + progPath + "\" " + task.options + " &";
                }
            #else
                cmd = "\"" + progPath + "\" " + task.options + " &";
            #endif

            Logger::instance().debug("Запуск: " + cmd);
            int ret = system(cmd.c_str());

            if (ret != 0) {
                result.success = false;
                result.message = "Программа завершилась с ошибкой или не запустилась";
            } else {
                // --- ОЖИДАНИЕ ФАЙЛА result.txt ---
                std::string targetFile = m_config.getResultPath() + "/result.txt";
                bool found = false;
                
                // Ждем до 60 секунд (можно вынести в конфиг)
                for (int i = 0; i < 60; ++i) {
                    if (fs::exists(targetFile)) {
                        found = true;
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }

                if (found) {
                    // Копируем файл под уникальным именем, чтобы не было конфликтов
                    std::string uniqueResult = m_config.getResultPath() + "/result_" + task.sessionId + "_data.txt";
                    try {
                        fs::copy_file(targetFile, uniqueResult, fs::copy_options::overwrite_existing);
                        result.files.push_back(uniqueResult); // Добавляем в список отправки
                        
                        // Удаляем оригинальный файл, как просил заказчик
                        fs::remove(targetFile);
                        Logger::instance().info("Файл result.txt подхвачен и удален");
                    } catch (const fs::filesystem_error& e) {
                        Logger::instance().error("Ошибка при работе с result.txt: " + std::string(e.what()));
                    }
                } else {
                    result.message = "Программа запущена, но result.txt не появился вовремя";
                }
            }
        }
    }

    // --- ФИНАЛЬНЫЙ ОТЧЕТ (Текстовый лог выполнения) ---
    std::string reportDir = m_config.getResultPath();
    std::string reportName = reportDir + "/report_" + task.sessionId + ".txt";

    try {
        if (!fs::exists(reportDir)) fs::create_directories(reportDir);
        
        std::ofstream file(reportName);
        if (file.is_open()) {
            file << "UID: " << m_config.getUid() << "\n";
            file << "Session: " << task.sessionId << "\n";
            file << "Status: " << (result.success ? "SUCCESS" : "FAILED") << "\n";
            file << "Message: " << result.message << "\n";
            file.close();
            
            // Добавляем файл отчета самым первым в список
            result.files.insert(result.files.begin(), reportName);
        }
    } catch (...) {
        Logger::instance().error("Критическая ошибка при записи отчета");
    }

    return result;
}