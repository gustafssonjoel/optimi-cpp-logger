#include "optimi/logger/logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {

bool write_text_file(const std::filesystem::path& path, const std::string& content) {
    std::ofstream output(path);
    if (!output.is_open()) {
        return false;
    }
    output << content;
    return true;
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return {};
    }
    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

bool expect_true(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

bool expect_false(bool condition, const std::string& message) {
    return expect_true(!condition, message);
}

std::string current_date_stamp() {
    const auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
    localtime_r(&now_time, &local_tm);
    std::ostringstream output;
    output << std::put_time(&local_tm, "%Y%m%d");
    return output.str();
}

std::filesystem::path rotated_path_for_today(const std::filesystem::path& base_path) {
    return base_path.parent_path() /
           (base_path.stem().string() + "_" + current_date_stamp() + base_path.extension().string());
}

std::string make_config_json(const std::string& log_file_path, bool daily_rotation) {
    return std::string("{\n") +
           "  \"log_file_path\": \"" + log_file_path + "\",\n" +
           "  \"min_level\": \"info\",\n" +
           "  \"auto_flush\": true,\n" +
           "  \"append\": true,\n" +
           "  \"daily_rotation\": " + (daily_rotation ? "true" : "false") + "\n" +
           "}";
}

bool test_init_from_json_creates_parent_and_log_file_no_rotation(const std::filesystem::path& temp_dir) {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const auto log_file_path = temp_dir / "runtime" / "nested" / "app.log";
    const auto parent_dir = log_file_path.parent_path();
    const auto config_path = temp_dir / "init_valid.json";

    std::error_code ec;
    std::filesystem::remove_all(parent_dir, ec);

    if (!write_text_file(config_path, make_config_json(log_file_path.string(), false))) {
        std::cerr << "FAILED: could not write test config " << config_path << '\n';
        return false;
    }

    std::string error;
    const bool ok = logger.init_from_json_file(config_path.string(), error);

    bool pass = true;
    pass &= expect_true(ok, "init_from_json_file should succeed for valid config without rotation");
    pass &= expect_true(error.empty(), "error should be empty on success");
    pass &= expect_true(logger.is_initialized(), "logger should be initialized");
    pass &= expect_true(std::filesystem::exists(parent_dir), "parent directory should be created");
    pass &= expect_true(std::filesystem::exists(log_file_path), "base log file should be created/opened");

    logger.shutdown();
    pass &= expect_false(logger.is_initialized(), "logger should report not initialized after shutdown");
    return pass;
}

bool test_init_from_json_creates_dated_file_with_rotation(const std::filesystem::path& temp_dir) {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const auto log_file_path = temp_dir / "runtime" / "rotating" / "app.log";
    const auto parent_dir = log_file_path.parent_path();
    const auto expected_rotated_path = rotated_path_for_today(log_file_path);
    const auto config_path = temp_dir / "init_rotation.json";

    std::error_code ec;
    std::filesystem::remove_all(parent_dir, ec);

    if (!write_text_file(config_path, make_config_json(log_file_path.string(), true))) {
        std::cerr << "FAILED: could not write test config " << config_path << '\n';
        return false;
    }

    std::string error;
    const bool ok = logger.init_from_json_file(config_path.string(), error);

    bool pass = true;
    pass &= expect_true(ok, "init_from_json_file should succeed for valid config with rotation");
    pass &= expect_true(error.empty(), "error should be empty on success");
    pass &= expect_true(logger.is_initialized(), "logger should be initialized");
    pass &= expect_true(std::filesystem::exists(parent_dir), "rotation parent directory should be created");
    pass &= expect_true(std::filesystem::exists(expected_rotated_path), "dated log file should be created/opened");

    logger.shutdown();
    pass &= expect_false(logger.is_initialized(), "logger should report not initialized after shutdown");
    return pass;
}

bool test_init_from_json_missing_file_fails(const std::filesystem::path& temp_dir) {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const auto config_path = temp_dir / "does_not_exist.json";
    std::string error;
    const bool ok = logger.init_from_json_file(config_path.string(), error);

    bool pass = true;
    pass &= expect_false(ok, "init_from_json_file should fail when config file is missing");
    pass &= expect_true(error.find("Failed to open config file") != std::string::npos, "error should mention config open failure");
    pass &= expect_false(logger.is_initialized(), "logger should remain not initialized on failure");
    return pass;
}

bool test_init_from_json_invalid_field_type_fails(const std::filesystem::path& temp_dir) {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const auto config_path = temp_dir / "invalid_type.json";
    const std::string json = R"({
  "log_file_path": "./logs/app.log",
  "append": "yes"
})";

    if (!write_text_file(config_path, json)) {
        std::cerr << "FAILED: could not write test config " << config_path << '\n';
        return false;
    }

    std::string error;
    const bool ok = logger.init_from_json_file(config_path.string(), error);

    bool pass = true;
    pass &= expect_false(ok, "init_from_json_file should fail for invalid field type");
    pass &= expect_true(error.find("Invalid type for append") != std::string::npos, "error should mention append type");
    pass &= expect_false(logger.is_initialized(), "logger should remain not initialized after parse failure");
    return pass;
}

bool test_log_writes_formatted_line(const std::filesystem::path& temp_dir) {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const auto log_file_path = temp_dir / "runtime" / "write" / "writer.log";
    const auto config_path = temp_dir / "write_valid.json";

    if (!write_text_file(config_path, make_config_json(log_file_path.string(), false))) {
        std::cerr << "FAILED: could not write test config " << config_path << '\n';
        return false;
    }

    std::string error;
    const bool ok = logger.init_from_json_file(config_path.string(), error);
    if (!ok) {
        std::cerr << "FAILED: init_from_json_file failed in write test: " << error << '\n';
        return false;
    }

    logger.info("formatted output check");
    logger.shutdown();

    const std::string content = read_text_file(log_file_path);
    bool pass = true;
    pass &= expect_true(!content.empty(), "log file should contain at least one line");
    pass &= expect_true(content.find("[INFO]") != std::string::npos, "log line should include INFO level");
    pass &= expect_true(content.find("[thread:") != std::string::npos, "log line should include thread id segment");
    pass &= expect_true(content.find("formatted output check") != std::string::npos, "log line should include message text");
    pass &= expect_true(content.find("logger_init_tests.cpp") != std::string::npos, "log line should include source file name");
    return pass;
}

bool test_macro_calls_write_expected_lines(const std::filesystem::path& temp_dir) {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const auto log_file_path = temp_dir / "runtime" / "macro" / "macro.log";
    const auto config_path = temp_dir / "macro_valid.json";

    if (!write_text_file(config_path, make_config_json(log_file_path.string(), false))) {
        std::cerr << "FAILED: could not write test config " << config_path << '\n';
        return false;
    }

    std::string error;
    const bool ok = logger.init_from_json_file(config_path.string(), error);
    if (!ok) {
        std::cerr << "FAILED: init_from_json_file failed in macro test: " << error << '\n';
        return false;
    }

    OPTIMI_LOG_INFO("macro info message");
    OPTIMI_LOG_ERROR("macro error message");
    logger.shutdown();

    const std::string content = read_text_file(log_file_path);
    bool pass = true;
    pass &= expect_true(!content.empty(), "macro log file should contain output");
    pass &= expect_true(content.find("[INFO]") != std::string::npos, "macro INFO line should be present");
    pass &= expect_true(content.find("[ERROR]") != std::string::npos, "macro ERROR line should be present");
    pass &= expect_true(content.find("macro info message") != std::string::npos, "macro info message should be present");
    pass &= expect_true(content.find("macro error message") != std::string::npos, "macro error message should be present");
    pass &= expect_true(content.find("logger_init_tests.cpp") != std::string::npos, "macro callsite file should be captured");
    return pass;
}

} // namespace

int main() {
    const auto temp_dir = std::filesystem::temp_directory_path() / "optimi_cpp_logger_init_tests";
    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
    std::filesystem::create_directories(temp_dir, ec);
    if (ec) {
        std::cerr << "FAILED: unable to create temp test directory: " << temp_dir << '\n';
        return 1;
    }

    bool all_passed = true;
    all_passed &= test_init_from_json_creates_parent_and_log_file_no_rotation(temp_dir);
    all_passed &= test_init_from_json_creates_dated_file_with_rotation(temp_dir);
    all_passed &= test_init_from_json_missing_file_fails(temp_dir);
    all_passed &= test_init_from_json_invalid_field_type_fails(temp_dir);
    all_passed &= test_log_writes_formatted_line(temp_dir);
    all_passed &= test_macro_calls_write_expected_lines(temp_dir);

    std::filesystem::remove_all(temp_dir, ec);
    return all_passed ? 0 : 1;
}
