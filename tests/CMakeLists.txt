cmake_minimum_required(VERSION 3.14)
find_package(GTest QUIET)

if(GTest_FOUND)
    # add_executable(integration_tests
    #     integration/server_availability_test.cpp
    #     integration/upload_results_test.cpp
    #     integration/backoff_test.cpp
    #     integration/polling_loop_test.cpp
    # )
    
    add_executable(unit_tests
        unit/uid_generation_test.cpp
        # unit/json_extractor_test.cpp
        unit/config_load_test.cpp
        ../src/Config.cpp 
        ../src/Logger.cpp 
    )

    # Пути к заголовочным файлам - применяем к unit_tests
    target_include_directories(unit_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/cpp-httplib
    )
    
    # target_link_libraries(integration_tests GTest::gtest_main)
    target_link_libraries(unit_tests GTest::gtest_main)
    
    enable_testing()
    # add_test(NAME integration_tests COMMAND integration_tests)
    add_test(NAME unit_tests COMMAND unit_tests)
endif()
