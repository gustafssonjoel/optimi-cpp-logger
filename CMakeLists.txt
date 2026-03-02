cmake_minimum_required(VERSION 3.20)
project(optimi_cpp_logger LANGUAGES CXX)

add_library(optimi_logger
    src/logger.cpp
)

target_include_directories(optimi_logger
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(optimi_logger PUBLIC cxx_std_20)
