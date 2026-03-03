#include "optimi/logger/logger.hpp"

#include <algorithm>
#include <cctype>

#include <nlohmann/json.hpp>

namespace {

using nlohmann::json;

std::string to_lower_copy(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

bool parse_log_level(const std::string& text, optimi::logger::LogLevel& out_level) {
    const std::string level_text = to_lower_copy(text);
    if (level_text == "trace") {
        out_level = optimi::logger::LogLevel::trace;
        return true;
    }
    if (level_text == "debug") {
        out_level = optimi::logger::LogLevel::debug;
        return true;
    }
    if (level_text == "info") {
        out_level = optimi::logger::LogLevel::info;
        return true;
    }
    if (level_text == "warn") {
        out_level = optimi::logger::LogLevel::warn;
        return true;
    }
    if (level_text == "error") {
        out_level = optimi::logger::LogLevel::error;
        return true;
    }
    if (level_text == "fatal") {
        out_level = optimi::logger::LogLevel::fatal;
        return true;
    }
    if (level_text == "off") {
        out_level = optimi::logger::LogLevel::off;
        return true;
    }
    return false;
}

} // namespace

namespace optimi::logger {

bool load_config_from_json_file(
    const std::string& json_path,
    LoggerConfig& out_config,
    std::string& error_message) {
    error_message.clear();

    std::ifstream config_stream(json_path);
    if (!config_stream.is_open()) {
        error_message = "Failed to open config file: " + json_path;
        return false;
    }

    json document;
    try {
        config_stream >> document;
    } catch (const json::parse_error& parse_error) {
        error_message = std::string("JSON parse error: ") + parse_error.what();
        return false;
    }

    if (!document.is_object()) {
        error_message = "Config JSON root must be an object.";
        return false;
    }

    LoggerConfig parsed_config{};

    if (!document.contains("log_file_path")) {
        error_message = "Missing required key: log_file_path";
        return false;
    }
    if (!document["log_file_path"].is_string()) {
        error_message = "Invalid type for log_file_path: expected string.";
        return false;
    }
    parsed_config.log_file_path = document["log_file_path"].get<std::string>();

    if (document.contains("min_level")) {
        if (!document["min_level"].is_string()) {
            error_message = "Invalid type for min_level: expected string.";
            return false;
        }
        if (!parse_log_level(document["min_level"].get<std::string>(), parsed_config.min_level)) {
            error_message = "Invalid min_level value.";
            return false;
        }
    }

    if (document.contains("console_min_level")) {
        if (!document["console_min_level"].is_string()) {
            error_message = "Invalid type for console_min_level: expected string.";
            return false;
        }
        if (!parse_log_level(document["console_min_level"].get<std::string>(), parsed_config.console_min_level)) {
            error_message = "Invalid console_min_level value.";
            return false;
        }
    }

    if (document.contains("auto_flush")) {
        if (!document["auto_flush"].is_boolean()) {
            error_message = "Invalid type for auto_flush: expected boolean.";
            return false;
        }
        parsed_config.auto_flush = document["auto_flush"].get<bool>();
    }

    if (document.contains("append")) {
        if (!document["append"].is_boolean()) {
            error_message = "Invalid type for append: expected boolean.";
            return false;
        }
        parsed_config.append = document["append"].get<bool>();
    }

    if (document.contains("daily_rotation")) {
        if (!document["daily_rotation"].is_boolean()) {
            error_message = "Invalid type for daily_rotation: expected boolean.";
            return false;
        }
        parsed_config.daily_rotation = document["daily_rotation"].get<bool>();
    }

    if (parsed_config.log_file_path.empty()) {
        error_message = "log_file_path must be non-empty.";
        return false;
    }

    parsed_config.config_json_path = json_path;
    out_config = parsed_config;
    return true;
}

} // namespace optimi::logger
