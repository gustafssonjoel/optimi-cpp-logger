#pragma once

#include <atomic>
#include <fstream>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>

namespace optimi::logger {

/**
 * @brief Severity level for log messages.
 */
enum class LogLevel {
    /** @brief Fine-grained diagnostic information. */
    trace,
    /** @brief Debug information useful during development. */
    debug,
    /** @brief General runtime information. */
    info,
    /** @brief Warning about a potential issue. */
    warn,
    /** @brief Error indicating an operation failed. */
    error,
    /** @brief Fatal error that may require process termination. */
    fatal,
    /** @brief Disable all logging output. */
    off
};

/**
 * @brief Runtime configuration for the logger.
 */
struct LoggerConfig {
    /** @brief Target log file path (base path used for rotation when enabled). */
    std::string log_file_path;
    /** @brief Minimum severity level that will be written to file. */
    LogLevel min_level = LogLevel::info;
    /** @brief Minimum severity level that will be written to console when console output is enabled. */
    LogLevel console_min_level = LogLevel::info;
    /** @brief Flush output stream after each write when true. */
    bool auto_flush = true;
    /** @brief Open file in append mode when true, otherwise truncate on init. */
    bool append = true;
    /** @brief Enable daily rotation using <base_name>_YYYYMMDD<extension>. */
    bool daily_rotation = true;
    /** @brief Source JSON path metadata when config was loaded from JSON. */
    std::string config_json_path;
};

/**
 * @brief Load logger configuration from a JSON file.
 * @param json_path Path to a JSON configuration file.
 * @param out_config Parsed configuration on success.
 * @param error_message Detailed error on failure.
 * @return true on success, false on parse/read/validation failure.
 */
bool load_config_from_json_file(
    const std::string& json_path,
    LoggerConfig& out_config,
    std::string& error_message);

/**
 * @brief Thread-safe singleton logger.
 */
class Logger {
public:
    /**
     * @brief Access the global logger instance.
     * @return Reference to the singleton logger.
     */
    static Logger& instance();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief Initialize logger with runtime configuration.
     * @param config Logger configuration values.
     * @return true on success, false if initialization fails.
     */
    bool init(const LoggerConfig& config);

    /**
     * @brief Load configuration from JSON and initialize logger.
     * @param json_path Path to JSON configuration file.
     * @param error_message Detailed error on failure.
     * @return true on success, false on read/parse/validation/init failure.
     */
    bool init_from_json_file(const std::string& json_path, std::string& error_message);

    /**
     * @brief Flush and close logger resources.
     *
     * Safe to call multiple times.
     */
    void shutdown();

    /**
     * @brief Update minimum log level.
     * @param level New minimum level.
     */
    void set_level(LogLevel level);

    /**
     * @brief Get the current minimum log level.
     * @return Current logger level.
     */
    LogLevel level() const;

    /**
     * @brief Check whether logger was initialized successfully.
     * @return true if initialized, otherwise false.
     */
    bool is_initialized() const;

    /**
     * @brief Determine whether a message should be logged for the current level.
     * @param message_level Message severity.
     * @return true if the message should be emitted, otherwise false.
     */
    bool should_log(LogLevel message_level) const;

    /**
     * @brief Write a log message.
     * @param message_level Message severity level.
     * @param message Message text.
     * @param file Source file path.
     * @param line Source line number.
     * @param function Source function name.
     */
    void log(
        LogLevel message_level,
        std::string_view message,
        const char* file,
        int line,
        const char* function);

    /**
     * @brief Log a trace-level message.
     * @param message Message text.
     * @param location Source location, defaults to call site.
     */
    void trace(
        std::string_view message,
        const std::source_location& location = std::source_location::current());

    /**
     * @brief Log a debug-level message.
     * @param message Message text.
     * @param location Source location, defaults to call site.
     */
    void debug(
        std::string_view message,
        const std::source_location& location = std::source_location::current());

    /**
     * @brief Log an info-level message.
     * @param message Message text.
     * @param location Source location, defaults to call site.
     */
    void info(
        std::string_view message,
        const std::source_location& location = std::source_location::current());

    /**
     * @brief Log a warn-level message.
     * @param message Message text.
     * @param location Source location, defaults to call site.
     */
    void warn(
        std::string_view message,
        const std::source_location& location = std::source_location::current());

    /**
     * @brief Log an error-level message.
     * @param message Message text.
     * @param location Source location, defaults to call site.
     */
    void error(
        std::string_view message,
        const std::source_location& location = std::source_location::current());

    /**
     * @brief Log a fatal-level message.
     * @param message Message text.
     * @param location Source location, defaults to call site.
     */
    void fatal(
        std::string_view message,
        const std::source_location& location = std::source_location::current());

private:
    Logger();
    ~Logger();

    bool should_log_unlocked(LogLevel message_level) const;

    mutable std::mutex mutex_;
    LoggerConfig config_;
    std::string current_date_stamp_;
    std::string active_log_path_;
    std::ofstream stream_;
    std::atomic<bool> initialized_;
};

#define OPTIMI_LOG_TRACE(message) ::optimi::logger::Logger::instance().trace((message))
#define OPTIMI_LOG_DEBUG(message) ::optimi::logger::Logger::instance().debug((message))
#define OPTIMI_LOG_INFO(message)  ::optimi::logger::Logger::instance().info((message))
#define OPTIMI_LOG_WARN(message)  ::optimi::logger::Logger::instance().warn((message))
#define OPTIMI_LOG_ERROR(message) ::optimi::logger::Logger::instance().error((message))
#define OPTIMI_LOG_FATAL(message) ::optimi::logger::Logger::instance().fatal((message))

} // namespace optimi::logger
