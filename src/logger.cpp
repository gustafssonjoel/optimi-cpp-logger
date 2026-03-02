#include "optimi/logger/logger.hpp"

namespace optimi::logger {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger()
    : config_{}, stream_{}, initialized_{false} {}

Logger::~Logger() {
    shutdown();
}

bool Logger::init(const LoggerConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    initialized_.store(false);
    return false;
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stream_.is_open()) {
        stream_.flush();
        stream_.close();
    }
    initialized_.store(false);
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.min_level = level;
}

LogLevel Logger::level() const {
    return config_.min_level;
}

bool Logger::is_initialized() const {
    return initialized_.load();
}

bool Logger::should_log(LogLevel message_level) const {
    return message_level >= config_.min_level && config_.min_level != LogLevel::off;
}

void Logger::log(
    LogLevel,
    std::string_view,
    const char*,
    int,
    const char*) {
}

void Logger::trace(
    std::string_view message,
    const std::source_location& location) {
    log(
        LogLevel::trace,
        message,
        location.file_name(),
        static_cast<int>(location.line()),
        location.function_name());
}

void Logger::debug(
    std::string_view message,
    const std::source_location& location) {
    log(
        LogLevel::debug,
        message,
        location.file_name(),
        static_cast<int>(location.line()),
        location.function_name());
}

void Logger::info(
    std::string_view message,
    const std::source_location& location) {
    log(
        LogLevel::info,
        message,
        location.file_name(),
        static_cast<int>(location.line()),
        location.function_name());
}

void Logger::warn(
    std::string_view message,
    const std::source_location& location) {
    log(
        LogLevel::warn,
        message,
        location.file_name(),
        static_cast<int>(location.line()),
        location.function_name());
}

void Logger::error(
    std::string_view message,
    const std::source_location& location) {
    log(
        LogLevel::error,
        message,
        location.file_name(),
        static_cast<int>(location.line()),
        location.function_name());
}

void Logger::fatal(
    std::string_view message,
    const std::source_location& location) {
    log(
        LogLevel::fatal,
        message,
        location.file_name(),
        static_cast<int>(location.line()),
        location.function_name());
}

} // namespace optimi::logger
