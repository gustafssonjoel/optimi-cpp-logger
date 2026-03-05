#include "optimi/logger/logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <ios>
#include <sstream>
#include <thread>

namespace {

std::string level_to_string(optimi::logger::LogLevel level) {
	switch (level) {
	case optimi::logger::LogLevel::trace:
		return "TRACE";
	case optimi::logger::LogLevel::debug:
		return "DEBUG";
	case optimi::logger::LogLevel::info:
		return "INFO";
	case optimi::logger::LogLevel::warn:
		return "WARN";
	case optimi::logger::LogLevel::error:
		return "ERROR";
	case optimi::logger::LogLevel::fatal:
		return "FATAL";
	case optimi::logger::LogLevel::off:
		return "OFF";
	}
	return "UNKNOWN";
}

const char* level_to_ansi_color(optimi::logger::LogLevel level) {
	switch (level) {
	case optimi::logger::LogLevel::trace:
		return "\033[90m"; // bright black / gray
	case optimi::logger::LogLevel::debug:
		return "\033[36m"; // cyan
	case optimi::logger::LogLevel::info:
		return "\033[32m"; // green
	case optimi::logger::LogLevel::warn:
		return "\033[33m"; // yellow
	case optimi::logger::LogLevel::error:
		return "\033[31m"; // red
	case optimi::logger::LogLevel::fatal:
		return "\033[35m"; // magenta
	case optimi::logger::LogLevel::off:
		return "\033[0m";
	}
	return "\033[0m";
}

std::string colorize_console_line(const std::string& line_text, optimi::logger::LogLevel level) {
	if (line_text.empty()) {
		return line_text;
	}

	const char* color = level_to_ansi_color(level);
	constexpr const char* reset = "\033[0m";
	if (line_text.back() == '\n') {
		return std::string(color) + line_text.substr(0, line_text.size() - 1) + reset + "\n";
	}
	return std::string(color) + line_text + reset;
}

std::tm current_local_time_tm() {
	auto now = std::chrono::system_clock::now();
	std::time_t now_time = std::chrono::system_clock::to_time_t(now);
	std::tm local_tm{};
	localtime_r(&now_time, &local_tm);
	return local_tm;
}

std::string current_date_stamp() {
	const std::tm local_tm = current_local_time_tm();
	std::ostringstream output;
	output << std::put_time(&local_tm, "%Y%m%d");
	return output.str();
}

std::string current_timestamp_with_millis() {
	auto now = std::chrono::system_clock::now();
	std::time_t now_time = std::chrono::system_clock::to_time_t(now);
	std::tm local_tm{};
	localtime_r(&now_time, &local_tm);

	const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
		now.time_since_epoch()) % 1000;

	std::ostringstream output;
	output << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S")
		<< "." << std::setw(3) << std::setfill('0') << millis.count();
	return output.str();
}

std::filesystem::path build_log_file_path(
	const std::string& configured_log_file_path,
	bool daily_rotation,
	const std::string& date_stamp) {
	std::filesystem::path base_path(configured_log_file_path);
	if (!daily_rotation) {
		return base_path;
	}

	const std::filesystem::path parent = base_path.parent_path();
	const std::string stem = base_path.stem().string();
	const std::string extension = base_path.extension().string();
	const std::string rotated_name = stem + "_" + date_stamp + extension;
	return parent / rotated_name;
}

bool open_log_stream(
	std::ofstream& stream,
	const std::filesystem::path& target_path,
	bool append_mode) {
	std::error_code fs_error;
	const std::filesystem::path parent_path = target_path.parent_path();
	if (!parent_path.empty()) {
		std::filesystem::create_directories(parent_path, fs_error);
		if (fs_error) {
			return false;
		}
	}

	std::ofstream new_stream;
	const std::ios_base::openmode open_mode = std::ios::out | (append_mode ? std::ios::app : std::ios::trunc);
	new_stream.open(target_path, open_mode);
	if (!new_stream.is_open()) {
		return false;
	}

	if (stream.is_open()) {
		stream.flush();
		stream.close();
	}
	stream = std::move(new_stream);
	return true;
}

bool should_log_against_level(optimi::logger::LogLevel message_level, optimi::logger::LogLevel minimum_level) {
	return message_level >= minimum_level && minimum_level != optimi::logger::LogLevel::off;
}

std::string short_source_file(const char* file) {
	if (file == nullptr || *file == '\0') {
		return "unknown";
	}

	const std::filesystem::path file_path(file);
	const std::filesystem::path filename = file_path.filename();
	if (filename.empty()) {
		return std::string(file);
	}
	return filename.string();
}

} // namespace

namespace optimi::logger {

Logger& Logger::instance() {
	static Logger logger;
	return logger;
}

Logger::Logger()
	: config_{}, current_date_stamp_{}, active_log_path_{}, stream_{}, initialized_{false} {}

Logger::~Logger() {
	shutdown();
}

bool Logger::init(const LoggerConfig& config) {
	std::lock_guard<std::mutex> lock(mutex_);

	if (config.log_file_path.empty()) {
		initialized_.store(false);
		return false;
	}

	const std::string initial_date = current_date_stamp();
	const std::filesystem::path target_path = build_log_file_path(
		config.log_file_path,
		config.daily_rotation,
		initial_date);

	if (!open_log_stream(stream_, target_path, config.append)) {
		initialized_.store(false);
		return false;
	}

	config_ = config;
	current_date_stamp_ = initial_date;
	active_log_path_ = target_path.string();
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
	current_date_stamp_.clear();
	active_log_path_.clear();
	initialized_.store(false);
}

void Logger::set_level(LogLevel level) {
	std::lock_guard<std::mutex> lock(mutex_);
	config_.min_level = level;
}

LogLevel Logger::level() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return config_.min_level;
}

bool Logger::is_initialized() const {
	return initialized_.load();
}

bool Logger::should_log(LogLevel message_level) const {
	std::lock_guard<std::mutex> lock(mutex_);
	return should_log_unlocked(message_level);
}

bool Logger::should_log_unlocked(LogLevel message_level) const {
	return should_log_against_level(message_level, config_.min_level);
}

void Logger::log(
	LogLevel message_level,
	std::string_view message,
	const char* file,
	int line,
	const char* function) {
	std::lock_guard<std::mutex> lock(mutex_);

	if (!initialized_.load()) {
		return;
	}

	bool should_log_to_file = should_log_unlocked(message_level);
	const bool should_log_to_console = should_log_against_level(message_level, config_.console_min_level);

	if (!should_log_to_file && !should_log_to_console) {
		return;
	}

	if (should_log_to_file && config_.daily_rotation) {
		const std::string now_date = current_date_stamp();
		if (now_date != current_date_stamp_) {
			const std::filesystem::path rotated_path = build_log_file_path(
				config_.log_file_path,
				true,
				now_date);
			if (!open_log_stream(stream_, rotated_path, config_.append)) {
				should_log_to_file = false;
			} else {
				current_date_stamp_ = now_date;
				active_log_path_ = rotated_path.string();
			}
		}
	}

	const std::string file_text = short_source_file(file);
	const char* function_text = (function != nullptr) ? function : "unknown";
	std::ostringstream line_builder;
	line_builder
		<< current_timestamp_with_millis()
		<< " [" << level_to_string(message_level) << "]";
	if (config_.show_thread_id) {
		line_builder << " [thread:" << std::this_thread::get_id() << "]";
	}
	line_builder
		<< " [" << file_text << ":" << line << " " << function_text << "] "
		<< message
		<< '\n';
	const std::string line_text = line_builder.str();

	if (should_log_to_file && stream_.is_open()) {
		stream_ << line_text;
	}

	if (should_log_to_console) {
		std::ostream& console_stream =
			(message_level >= LogLevel::error) ? static_cast<std::ostream&>(std::cerr) : static_cast<std::ostream&>(std::cout);
		if (config_.console_color) {
			console_stream << colorize_console_line(line_text, message_level);
		} else {
			console_stream << line_text;
		}
	}

	if (config_.auto_flush) {
		if (stream_.is_open()) {
			stream_.flush();
		}
		if (should_log_to_console) {
			std::cout.flush();
			std::cerr.flush();
		}
	}
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
