#include "optimi/logger/logger.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

bool expect_true(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

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

bool test_concurrent_logging_writes_all_lines(const std::filesystem::path& temp_dir) {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const auto log_file_path = temp_dir / "threaded" / "concurrent.log";

    optimi::logger::LoggerConfig config{};
    config.log_file_path = log_file_path.string();
    config.min_level = optimi::logger::LogLevel::trace;
    config.auto_flush = true;
    config.append = false;
    config.daily_rotation = false;

    if (!logger.init(config)) {
        std::cerr << "FAILED: logger.init failed for thread safety test\n";
        return false;
    }

    constexpr int thread_count = 8;
    constexpr int messages_per_thread = 200;
    const int expected_lines = thread_count * messages_per_thread;

    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (int thread_index = 0; thread_index < thread_count; ++thread_index) {
        workers.emplace_back([thread_index]() {
            for (int message_index = 0; message_index < messages_per_thread; ++message_index) {
                optimi::logger::Logger::instance().info(
                    "thread=" + std::to_string(thread_index) + " message=" + std::to_string(message_index));
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    logger.shutdown();

    const std::size_t line_count = count_lines_in_file(log_file_path);
    bool pass = true;
    pass &= expect_true(line_count == static_cast<std::size_t>(expected_lines), "line count should match all concurrent writes");
    return pass;
}

} // namespace

int main() {
    const auto temp_dir = std::filesystem::temp_directory_path() / "optimi_cpp_logger_thread_tests";
    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
    std::filesystem::create_directories(temp_dir, ec);
    if (ec) {
        std::cerr << "FAILED: unable to create temp test directory: " << temp_dir << '\n';
        return 1;
    }

    bool all_passed = true;
    all_passed &= test_concurrent_logging_writes_all_lines(temp_dir);

    std::filesystem::remove_all(temp_dir, ec);
    return all_passed ? 0 : 1;
}
