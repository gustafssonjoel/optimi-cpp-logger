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

std::string make_config_json(const std::string& log_file_path) {
    return std::string("{\n") +
           "  \"log_file_path\": \"" + log_file_path + "\",\n" +
           "  \"min_level\": \"info\",\n" +
           "  \"auto_flush\": true,\n" +
           "  \"append\": true,\n" +
           "  \"daily_rotation\": true\n" +
           "}";
}

bool test_init_from_json_creates_parent_and_log_file(const std::filesystem::path& temp_dir) {
    auto& logger = optimi::logger::Logger::instance();
    logger.shutdown();

    const auto log_file_path = temp_dir / "runtime" / "nested" / "app.log";
    const auto parent_dir = log_file_path.parent_path();
    const auto config_path = temp_dir / "init_valid.json";

    std::error_code ec;
    std::filesystem::remove_all(parent_dir, ec);

    if (!write_text_file(config_path, make_config_json(log_file_path.string()))) {
        std::cerr << "FAILED: could not write test config " << config_path << '\n';
        return false;
    }

    std::string error;
    const bool ok = logger.init_from_json_file(config_path.string(), error);

    bool pass = true;
    pass &= expect_true(ok, "init_from_json_file should succeed for valid config");
    pass &= expect_true(error.empty(), "error should be empty on success");
    pass &= expect_true(logger.is_initialized(), "logger should be initialized");
    pass &= expect_true(std::filesystem::exists(parent_dir), "parent directory should be created");
    pass &= expect_true(std::filesystem::exists(log_file_path), "log file should be created/opened");

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
    all_passed &= test_init_from_json_creates_parent_and_log_file(temp_dir);
    all_passed &= test_init_from_json_missing_file_fails(temp_dir);
    all_passed &= test_init_from_json_invalid_field_type_fails(temp_dir);

    std::filesystem::remove_all(temp_dir, ec);
    return all_passed ? 0 : 1;
}
