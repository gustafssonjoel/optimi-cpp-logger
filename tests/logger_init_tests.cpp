#include "optimi/logger/logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <streambuf>
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

bool test_different_file_and_console_levels(const std::filesystem::path& temp_dir) {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const auto log_file_path = temp_dir / "runtime" / "split_levels" / "split.log";

    optimi::logger::LoggerConfig config{};
    config.log_file_path = log_file_path.string();
    config.min_level = optimi::logger::LogLevel::error;
    config.console_min_level = optimi::logger::LogLevel::info;
    config.auto_flush = true;
    config.append = false;
    config.daily_rotation = false;

    if (!logger.init(config)) {
        std::cerr << "FAILED: logger.init failed in split level test\n";
        return false;
    }

    std::ostringstream captured_stdout;
    std::ostringstream captured_stderr;
    std::streambuf* original_stdout = std::cout.rdbuf(captured_stdout.rdbuf());
    std::streambuf* original_stderr = std::cerr.rdbuf(captured_stderr.rdbuf());

    logger.info("console only message");
    logger.error("console and file message");

    std::cout.rdbuf(original_stdout);
    std::cerr.rdbuf(original_stderr);
    logger.shutdown();

    const std::string file_content = read_text_file(log_file_path);
    const std::string stdout_content = captured_stdout.str();
    const std::string stderr_content = captured_stderr.str();

    bool pass = true;
    pass &= expect_true(stdout_content.find("console only message") != std::string::npos, "console should include info message");
    pass &= expect_true(stderr_content.find("console and file message") != std::string::npos, "stderr should include error message");

    pass &= expect_true(file_content.find("console only message") == std::string::npos, "file should not include info below file level");
    pass &= expect_true(file_content.find("console and file message") != std::string::npos, "file should include error at file level");
    return pass;
}

bool test_off_levels_disable_file_and_console_output(const std::filesystem::path& temp_dir) {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const auto log_file_path = temp_dir / "runtime" / "off_levels" / "off.log";

    optimi::logger::LoggerConfig config{};
    config.log_file_path = log_file_path.string();
    config.min_level = optimi::logger::LogLevel::off;
    config.console_min_level = optimi::logger::LogLevel::off;
    config.auto_flush = true;
    config.append = false;
    config.daily_rotation = false;

    if (!logger.init(config)) {
        std::cerr << "FAILED: logger.init failed in off-level test\n";
        return false;
    }

    std::ostringstream captured_stdout;
    std::ostringstream captured_stderr;
    std::streambuf* original_stdout = std::cout.rdbuf(captured_stdout.rdbuf());
    std::streambuf* original_stderr = std::cerr.rdbuf(captured_stderr.rdbuf());

    logger.info("should not be logged");
    logger.error("should also not be logged");

    std::cout.rdbuf(original_stdout);
    std::cerr.rdbuf(original_stderr);
    logger.shutdown();

    const std::string file_content = read_text_file(log_file_path);
    const std::string stdout_content = captured_stdout.str();
    const std::string stderr_content = captured_stderr.str();

    bool pass = true;
    pass &= expect_true(stdout_content.empty(), "stdout should be empty when console_min_level is off");
    pass &= expect_true(stderr_content.empty(), "stderr should be empty when console_min_level is off");
    pass &= expect_true(file_content.empty(), "file should contain no lines when min_level is off");
    return pass;
}

bool test_console_output_uses_level_colors_when_enabled(const std::filesystem::path& temp_dir) {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const auto log_file_path = temp_dir / "runtime" / "console_color" / "color.log";

    optimi::logger::LoggerConfig config{};
    config.log_file_path = log_file_path.string();
    config.min_level = optimi::logger::LogLevel::off;
    config.console_min_level = optimi::logger::LogLevel::info;
    config.console_color = true;
    config.auto_flush = true;
    config.append = false;
    config.daily_rotation = false;

    if (!logger.init(config)) {
        std::cerr << "FAILED: logger.init failed in console color test\n";
        return false;
    }

    std::ostringstream captured_stdout;
    std::streambuf* original_stdout = std::cout.rdbuf(captured_stdout.rdbuf());

    logger.info("colored info");

    std::cout.rdbuf(original_stdout);
    logger.shutdown();

    const std::string stdout_content = captured_stdout.str();
    bool pass = true;
    pass &= expect_true(stdout_content.find("\033[32m") != std::string::npos, "info console line should include green ANSI color");
    pass &= expect_true(stdout_content.find("\033[0m") != std::string::npos, "console line should include ANSI reset code");
    pass &= expect_true(stdout_content.find("colored info") != std::string::npos, "console line should include message text");
    return pass;
}

bool test_console_output_has_no_colors_when_disabled(const std::filesystem::path& temp_dir) {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const auto log_file_path = temp_dir / "runtime" / "console_plain" / "plain.log";

    optimi::logger::LoggerConfig config{};
    config.log_file_path = log_file_path.string();
    config.min_level = optimi::logger::LogLevel::off;
    config.console_min_level = optimi::logger::LogLevel::info;
    config.console_color = false;
    config.auto_flush = true;
    config.append = false;
    config.daily_rotation = false;

    if (!logger.init(config)) {
        std::cerr << "FAILED: logger.init failed in console plain test\n";
        return false;
    }

    std::ostringstream captured_stdout;
    std::streambuf* original_stdout = std::cout.rdbuf(captured_stdout.rdbuf());

    logger.info("plain info");

    std::cout.rdbuf(original_stdout);
    logger.shutdown();

    const std::string stdout_content = captured_stdout.str();
    bool pass = true;
    pass &= expect_true(stdout_content.find("plain info") != std::string::npos, "console line should include plain message text");
    pass &= expect_true(stdout_content.find("\033[") == std::string::npos, "console line should not contain ANSI color codes when disabled");
    return pass;
}

bool test_show_thread_id_false_omits_thread_segment_in_file_and_console(const std::filesystem::path& temp_dir) {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const auto log_file_path = temp_dir / "runtime" / "show_thread_id_false" / "hide.log";

    optimi::logger::LoggerConfig config{};
    config.log_file_path = log_file_path.string();
    config.min_level = optimi::logger::LogLevel::info;
    config.console_min_level = optimi::logger::LogLevel::info;
    config.console_color = false;
    config.auto_flush = true;
    config.append = false;
    config.daily_rotation = false;
    config.show_thread_id = false;

    if (!logger.init(config)) {
        std::cerr << "FAILED: logger.init failed in show_thread_id test\n";
        return false;
    }

    std::ostringstream captured_stdout;
    std::streambuf* original_stdout = std::cout.rdbuf(captured_stdout.rdbuf());

    logger.info("hidden thread id message");

    std::cout.rdbuf(original_stdout);
    logger.shutdown();

    const std::string file_content = read_text_file(log_file_path);
    const std::string stdout_content = captured_stdout.str();

    bool pass = true;
    pass &= expect_true(file_content.find("hidden thread id message") != std::string::npos, "file should include logged message");
    pass &= expect_true(stdout_content.find("hidden thread id message") != std::string::npos, "console should include logged message");
    pass &= expect_true(file_content.find("[thread:") == std::string::npos, "file line should omit thread id segment when show_thread_id is false");
    pass &= expect_true(stdout_content.find("[thread:") == std::string::npos, "console line should omit thread id segment when show_thread_id is false");
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
    all_passed &= test_different_file_and_console_levels(temp_dir);
    all_passed &= test_off_levels_disable_file_and_console_output(temp_dir);
    all_passed &= test_console_output_uses_level_colors_when_enabled(temp_dir);
    all_passed &= test_console_output_has_no_colors_when_disabled(temp_dir);
    all_passed &= test_show_thread_id_false_omits_thread_segment_in_file_and_console(temp_dir);

    std::filesystem::remove_all(temp_dir, ec);
    return all_passed ? 0 : 1;
}
