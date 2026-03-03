#include "optimi/logger/logger.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

std::size_t count_lines_in_file(const std::filesystem::path& file_path) {
    std::ifstream input(file_path);
    if (!input.is_open()) {
        return 0;
    }

    std::size_t lines = 0;
    std::string line;
    while (std::getline(input, line)) {
        ++lines;
    }
    return lines;
}

} // namespace

int main() {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const std::filesystem::path log_file_path = "./example_logs/multi_thread_example.log";

    optimi::logger::LoggerConfig config{};
    config.log_file_path = log_file_path.string();
    config.min_level = optimi::logger::LogLevel::info;
    config.auto_flush = true;
    config.append = false;
    config.daily_rotation = false;

    if (!logger.init(config)) {
        std::cerr << "Failed to initialize logger for multi-thread example\n";
        return 1;
    }

    constexpr int worker_count = 6;
    constexpr int messages_per_worker = 120;
    const int expected_lines = worker_count * messages_per_worker;

    std::vector<std::thread> workers;
    workers.reserve(worker_count);

    for (int worker_index = 0; worker_index < worker_count; ++worker_index) {
        workers.emplace_back([worker_index]() {
            for (int message_index = 0; message_index < messages_per_worker; ++message_index) {
                OPTIMI_LOG_INFO(
                    "worker=" + std::to_string(worker_index) +
                    " message=" + std::to_string(message_index));
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    logger.shutdown();

    if (!std::filesystem::exists(log_file_path)) {
        std::cerr << "Expected log file was not created: " << log_file_path << '\n';
        return 2;
    }

    const std::size_t line_count = count_lines_in_file(log_file_path);
    if (line_count != static_cast<std::size_t>(expected_lines)) {
        std::cerr << "Unexpected line count in log file. expected=" << expected_lines
                  << " actual=" << line_count << '\n';
        return 3;
    }

    std::cout << "Multi-thread example completed successfully. Log: " << log_file_path << '\n';
    return 0;
}
