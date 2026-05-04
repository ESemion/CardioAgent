cmake_minimum_required(VERSION 3.10)
project(CardioAgent)

# Подавляем предупреждения от устаревших методов в CMakeLists
add_compile_options(-Wno-deprecated-declarations)

set(CMAKE_CXX_STANDARD 17)

# Ищем OpenSSL
find_package(OpenSSL REQUIRED)

# Добавляем исполняемый файл
add_executable(cardioagent
    src/main.cpp
    src/Config.cpp
    src/Agent.cpp
    src/TaskExecutor.cpp
    src/Logger.cpp
    src/ServerClient.cpp
    
)
add_subdirectory(tests)


# Пути к заголовочным файлам
target_include_directories(cardioagent PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/cpp-httplib
)

# Линковка с OpenSSL
target_link_libraries(cardioagent OpenSSL::SSL OpenSSL::Crypto)

# macOS: линковка с Security и CoreFoundation для загрузки системных сертификатов
if(APPLE)
    target_link_libraries(cardioagent "-framework CoreFoundation" "-framework Security")
endif()


