cmake_minimum_required(VERSION 3.20)
project(optimi_cpp_logger LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(optimi_logger
    src/config_parser.cpp
    src/logger.cpp
)

target_include_directories(optimi_logger
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(optimi_logger PUBLIC cxx_std_20)

target_link_libraries(optimi_logger
    PUBLIC
        nlohmann_json::nlohmann_json
)

add_executable(multi_thread_example
    examples/multi_thread_example.cpp
)

target_link_libraries(multi_thread_example
    PRIVATE
        optimi_logger
)

enable_testing()

add_executable(config_parser_tests
    tests/config_parser_tests.cpp
)

target_link_libraries(config_parser_tests
    PRIVATE
        optimi_logger
)

add_test(
    NAME config_parser_tests
    COMMAND config_parser_tests
)

add_executable(logger_init_tests
    tests/logger_init_tests.cpp
)

target_link_libraries(logger_init_tests
    PRIVATE
        optimi_logger
)

add_test(
    NAME logger_init_tests
    COMMAND logger_init_tests
)

add_executable(thread_safety_tests
    tests/thread_safety_tests.cpp
)

target_link_libraries(thread_safety_tests
    PRIVATE
        optimi_logger
)

add_test(
    NAME thread_safety_tests
    COMMAND thread_safety_tests
)

add_test(
    NAME multi_thread_example_test
    COMMAND multi_thread_example
)
