#include "optimi/logger/logger.hpp"

#include <filesystem>
#include <ios>

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

	if (config.log_file_path.empty()) {
		initialized_.store(false);
		return false;
	}

	std::filesystem::path file_path(config.log_file_path);
	const std::filesystem::path parent_path = file_path.parent_path();

	std::error_code fs_error;
	if (!parent_path.empty()) {
		std::filesystem::create_directories(parent_path, fs_error);
		if (fs_error) {
			initialized_.store(false);
			return false;
		}
	}

	if (stream_.is_open()) {
		stream_.flush();
		stream_.close();
	}

	const std::ios_base::openmode open_mode = std::ios::out | (config.append ? std::ios::app : std::ios::trunc);
	stream_.open(file_path, open_mode);
	if (!stream_.is_open()) {
		initialized_.store(false);
		return false;
	}

	config_ = config;
	initialized_.store(true);
	return true;
}

bool Logger::init_from_json_file(const std::string& json_path, std::string& error_message) {
	LoggerConfig parsed_config;
	if (!load_config_from_json_file(json_path, parsed_config, error_message)) {
		return false;
	}
	if (!init(parsed_config)) {
		error_message = "Failed to initialize logger from parsed configuration.";
		return false;
	}
	error_message.clear();
	return true;
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
