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
