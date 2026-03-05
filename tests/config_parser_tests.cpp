#include "optimi/logger/logger.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
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

bool test_valid_full_config(const std::filesystem::path& temp_dir) {
    const auto config_path = temp_dir / "valid_full.json";
    const std::string json = R"({
  "log_file_path": "./logs/app.log",
  "min_level": "debug",
  "console_min_level": "warn",
  "auto_flush": false,
  "append": false,
  "daily_rotation": false,
  "console_color": false,
  "show_thread_id": false
})";

    if (!write_text_file(config_path, json)) {
        std::cerr << "FAILED: could not write test file " << config_path << '\n';
        return false;
    }

    optimi::logger::LoggerConfig config{};
    std::string error;
    const bool ok = optimi::logger::load_config_from_json_file(config_path.string(), config, error);

    bool pass = true;
    pass &= expect_true(ok, "valid config should parse");
    pass &= expect_true(error.empty(), "error must be empty on success");
    pass &= expect_true(config.log_file_path == "./logs/app.log", "log_file_path should match input");
    pass &= expect_true(config.min_level == optimi::logger::LogLevel::debug, "min_level should parse to debug");
    pass &= expect_true(config.console_min_level == optimi::logger::LogLevel::warn, "console_min_level should parse to warn");
    pass &= expect_true(config.auto_flush == false, "auto_flush should parse false");
    pass &= expect_true(config.append == false, "append should parse false");
    pass &= expect_true(config.daily_rotation == false, "daily_rotation should parse false");
    pass &= expect_true(config.console_color == false, "console_color should parse false");
    pass &= expect_true(config.show_thread_id == false, "show_thread_id should parse false");
    pass &= expect_true(config.config_json_path == config_path.string(), "config_json_path should be set to input path");
    return pass;
}

bool test_defaults_when_optional_missing(const std::filesystem::path& temp_dir) {
    const auto config_path = temp_dir / "valid_defaults.json";
    const std::string json = R"({
  "log_file_path": "./logs/default.log"
})";

    if (!write_text_file(config_path, json)) {
        std::cerr << "FAILED: could not write test file " << config_path << '\n';
        return false;
    }

    optimi::logger::LoggerConfig config{};
    std::string error;
    const bool ok = optimi::logger::load_config_from_json_file(config_path.string(), config, error);

    bool pass = true;
    pass &= expect_true(ok, "config with only required fields should parse");
    pass &= expect_true(config.min_level == optimi::logger::LogLevel::info, "default min_level should be info");
    pass &= expect_true(config.console_min_level == optimi::logger::LogLevel::info, "default console_min_level should be info");
    pass &= expect_true(config.auto_flush == true, "default auto_flush should be true");
    pass &= expect_true(config.append == true, "default append should be true");
    pass &= expect_true(config.daily_rotation == true, "default daily_rotation should be true");
    pass &= expect_true(config.console_color == true, "default console_color should be true");
    pass &= expect_true(config.show_thread_id == true, "default show_thread_id should be true");
    return pass;
}

bool test_missing_required_key_fails(const std::filesystem::path& temp_dir) {
    const auto config_path = temp_dir / "missing_key.json";
    const std::string json = R"({
  "min_level": "info"
})";

    if (!write_text_file(config_path, json)) {
        std::cerr << "FAILED: could not write test file " << config_path << '\n';
        return false;
    }

    optimi::logger::LoggerConfig config{};
    std::string error;
    const bool ok = optimi::logger::load_config_from_json_file(config_path.string(), config, error);

    bool pass = true;
    pass &= expect_false(ok, "missing log_file_path should fail");
    pass &= expect_true(error.find("Missing required key") != std::string::npos, "error should mention missing required key");
    return pass;
}

bool test_invalid_min_level_fails(const std::filesystem::path& temp_dir) {
    const auto config_path = temp_dir / "invalid_min_level.json";
    const std::string json = R"({
  "log_file_path": "./logs/app.log",
  "min_level": "verbose"
})";

    if (!write_text_file(config_path, json)) {
        std::cerr << "FAILED: could not write test file " << config_path << '\n';
        return false;
    }

    optimi::logger::LoggerConfig config{};
    std::string error;
    const bool ok = optimi::logger::load_config_from_json_file(config_path.string(), config, error);

    bool pass = true;
    pass &= expect_false(ok, "invalid min_level should fail");
    pass &= expect_true(error.find("Invalid min_level") != std::string::npos, "error should mention invalid min_level");
    return pass;
}

bool test_invalid_type_fails(const std::filesystem::path& temp_dir) {
    const auto config_path = temp_dir / "invalid_type.json";
    const std::string json = R"({
  "log_file_path": "./logs/app.log",
  "auto_flush": "yes"
})";

    if (!write_text_file(config_path, json)) {
        std::cerr << "FAILED: could not write test file " << config_path << '\n';
        return false;
    }

    optimi::logger::LoggerConfig config{};
    std::string error;
    const bool ok = optimi::logger::load_config_from_json_file(config_path.string(), config, error);

    bool pass = true;
    pass &= expect_false(ok, "invalid auto_flush type should fail");
    pass &= expect_true(error.find("Invalid type for auto_flush") != std::string::npos, "error should mention auto_flush type");
    return pass;
}

bool test_invalid_console_min_level_value_fails(const std::filesystem::path& temp_dir) {
    const auto config_path = temp_dir / "invalid_console_min_level_value.json";
    const std::string json = R"({
  "log_file_path": "./logs/app.log",
  "console_min_level": "verbose"
})";

    if (!write_text_file(config_path, json)) {
        std::cerr << "FAILED: could not write test file " << config_path << '\n';
        return false;
    }

    optimi::logger::LoggerConfig config{};
    std::string error;
    const bool ok = optimi::logger::load_config_from_json_file(config_path.string(), config, error);

    bool pass = true;
    pass &= expect_false(ok, "invalid console_min_level value should fail");
    pass &= expect_true(error.find("Invalid console_min_level value") != std::string::npos, "error should mention invalid console_min_level value");
    return pass;
}

bool test_invalid_console_color_type_fails(const std::filesystem::path& temp_dir) {
    const auto config_path = temp_dir / "invalid_console_color_type.json";
    const std::string json = R"({
  "log_file_path": "./logs/app.log",
  "console_color": "yes"
})";

    if (!write_text_file(config_path, json)) {
        std::cerr << "FAILED: could not write test file " << config_path << '\n';
        return false;
    }

    optimi::logger::LoggerConfig config{};
    std::string error;
    const bool ok = optimi::logger::load_config_from_json_file(config_path.string(), config, error);

    bool pass = true;
    pass &= expect_false(ok, "invalid console_color type should fail");
    pass &= expect_true(error.find("Invalid type for console_color") != std::string::npos, "error should mention console_color type");
    return pass;
}

bool test_invalid_show_thread_id_type_fails(const std::filesystem::path& temp_dir) {
        const auto config_path = temp_dir / "invalid_show_thread_id_type.json";
    const std::string json = R"({
  "log_file_path": "./logs/app.log",
    "show_thread_id": "yes"
})";

    if (!write_text_file(config_path, json)) {
        std::cerr << "FAILED: could not write test file " << config_path << '\n';
        return false;
    }

    optimi::logger::LoggerConfig config{};
    std::string error;
    const bool ok = optimi::logger::load_config_from_json_file(config_path.string(), config, error);

    bool pass = true;
    pass &= expect_false(ok, "invalid show_thread_id type should fail");
    pass &= expect_true(error.find("Invalid type for show_thread_id") != std::string::npos, "error should mention show_thread_id type");
    return pass;
}

bool test_off_levels_parse_successfully(const std::filesystem::path& temp_dir) {
    const auto config_path = temp_dir / "off_levels.json";
    const std::string json = R"({
  "log_file_path": "./logs/off.log",
  "min_level": "off",
  "console_min_level": "off"
})";

    if (!write_text_file(config_path, json)) {
        std::cerr << "FAILED: could not write test file " << config_path << '\n';
        return false;
    }

    optimi::logger::LoggerConfig config{};
    std::string error;
    const bool ok = optimi::logger::load_config_from_json_file(config_path.string(), config, error);

    bool pass = true;
    pass &= expect_true(ok, "off levels should parse successfully");
    pass &= expect_true(error.empty(), "error should be empty for off-level parse");
    pass &= expect_true(config.min_level == optimi::logger::LogLevel::off, "min_level should parse to off");
    pass &= expect_true(config.console_min_level == optimi::logger::LogLevel::off, "console_min_level should parse to off");
    return pass;
}

bool test_parse_error_fails(const std::filesystem::path& temp_dir) {
    const auto config_path = temp_dir / "parse_error.json";
    const std::string json = R"({ "log_file_path": "./logs/app.log", )";

    if (!write_text_file(config_path, json)) {
        std::cerr << "FAILED: could not write test file " << config_path << '\n';
        return false;
    }

    optimi::logger::LoggerConfig config{};
    std::string error;
    const bool ok = optimi::logger::load_config_from_json_file(config_path.string(), config, error);

    bool pass = true;
    pass &= expect_false(ok, "invalid json syntax should fail");
    pass &= expect_true(error.find("JSON parse error") != std::string::npos, "error should mention parse error");
    return pass;
}

} // namespace

int main() {
    const auto temp_dir = std::filesystem::temp_directory_path() / "optimi_cpp_logger_config_tests";
    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
    std::filesystem::create_directories(temp_dir, ec);
    if (ec) {
        std::cerr << "FAILED: unable to create temp test directory: " << temp_dir << '\n';
        return 1;
    }

    bool all_passed = true;
    all_passed &= test_valid_full_config(temp_dir);
    all_passed &= test_defaults_when_optional_missing(temp_dir);
    all_passed &= test_missing_required_key_fails(temp_dir);
    all_passed &= test_invalid_min_level_fails(temp_dir);
    all_passed &= test_invalid_type_fails(temp_dir);
    all_passed &= test_invalid_console_min_level_value_fails(temp_dir);
    all_passed &= test_invalid_console_color_type_fails(temp_dir);
    all_passed &= test_invalid_show_thread_id_type_fails(temp_dir);
    all_passed &= test_off_levels_parse_successfully(temp_dir);
    all_passed &= test_parse_error_fails(temp_dir);

    std::filesystem::remove_all(temp_dir, ec);
    return all_passed ? 0 : 1;
}
