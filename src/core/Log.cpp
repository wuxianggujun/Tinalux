#include "tinalux/core/Log.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#if defined(_WIN32) && defined(_MSC_VER)
#include <spdlog/sinks/msvc_sink.h>
#endif
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace tinalux::core {

namespace {

struct LoggerState {
    std::mutex mutex;
    std::shared_ptr<spdlog::logger> logger;
    LogConfig config;
    bool initialized = false;
};

LoggerState& loggerState()
{
    static LoggerState state;
    return state;
}

std::string toLowerCopy(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

spdlog::level::level_enum toSpdlogLevel(LogLevel level)
{
    switch (level) {
    case LogLevel::Trace:
        return spdlog::level::trace;
    case LogLevel::Debug:
        return spdlog::level::debug;
    case LogLevel::Info:
        return spdlog::level::info;
    case LogLevel::Warn:
        return spdlog::level::warn;
    case LogLevel::Error:
        return spdlog::level::err;
    case LogLevel::Critical:
        return spdlog::level::critical;
    case LogLevel::Off:
    default:
        return spdlog::level::off;
    }
}

LogLevel fromSpdlogLevel(spdlog::level::level_enum level)
{
    switch (level) {
    case spdlog::level::trace:
        return LogLevel::Trace;
    case spdlog::level::debug:
        return LogLevel::Debug;
    case spdlog::level::info:
        return LogLevel::Info;
    case spdlog::level::warn:
        return LogLevel::Warn;
    case spdlog::level::err:
        return LogLevel::Error;
    case spdlog::level::critical:
        return LogLevel::Critical;
    case spdlog::level::off:
    default:
        return LogLevel::Off;
    }
}

std::optional<bool> parseBool(std::string value)
{
    value = toLowerCopy(std::move(value));
    if (value == "1" || value == "true" || value == "yes" || value == "on") {
        return true;
    }
    if (value == "0" || value == "false" || value == "no" || value == "off") {
        return false;
    }
    return std::nullopt;
}

std::optional<LogLevel> parseLogLevel(std::string value)
{
    value = toLowerCopy(std::move(value));
    if (value == "trace") {
        return LogLevel::Trace;
    }
    if (value == "debug") {
        return LogLevel::Debug;
    }
    if (value == "info") {
        return LogLevel::Info;
    }
    if (value == "warn" || value == "warning") {
        return LogLevel::Warn;
    }
    if (value == "error" || value == "err") {
        return LogLevel::Error;
    }
    if (value == "critical" || value == "fatal") {
        return LogLevel::Critical;
    }
    if (value == "off") {
        return LogLevel::Off;
    }
    return std::nullopt;
}

std::optional<std::string> getEnvString(const char* name)
{
    if (name == nullptr) {
        return std::nullopt;
    }

    const char* value = std::getenv(name);
    if (value == nullptr) {
        return std::nullopt;
    }

    return std::string(value);
}

std::optional<std::size_t> parseSize(std::string value)
{
    try {
        std::size_t parsedCharacters = 0;
        const unsigned long long parsed = std::stoull(value, &parsedCharacters, 10);
        if (parsedCharacters != value.size()) {
            return std::nullopt;
        }
        return static_cast<std::size_t>(parsed);
    } catch (...) {
        return std::nullopt;
    }
}

LogConfig applyEnvironmentOverrides(LogConfig config)
{
    if (const auto value = getEnvString("TINALUX_LOGGER_NAME"); value && !value->empty()) {
        config.loggerName = *value;
    }

    if (const auto value = getEnvString("TINALUX_LOG_FILE_PATH"); value) {
        config.filePath = *value;
    }

    if (const auto value = getEnvString("TINALUX_LOG_CONSOLE_PATTERN")) {
        config.consolePattern = *value;
    }

    if (const auto value = getEnvString("TINALUX_LOG_PLAIN_PATTERN")) {
        config.plainPattern = *value;
    }

    if (const auto value = getEnvString("TINALUX_LOG_LEVEL")) {
        if (const auto parsed = parseLogLevel(*value)) {
            config.level = *parsed;
        }
    }

    if (const auto value = getEnvString("TINALUX_LOG_FLUSH_LEVEL")) {
        if (const auto parsed = parseLogLevel(*value)) {
            config.flushLevel = *parsed;
        }
    }

    if (const auto value = getEnvString("TINALUX_LOG_CONSOLE")) {
        if (const auto parsed = parseBool(*value)) {
            config.enableConsole = *parsed;
        }
    }

    if (const auto value = getEnvString("TINALUX_LOG_FILE")) {
        if (const auto parsed = parseBool(*value)) {
            config.enableFile = *parsed;
        }
    }

    if (const auto value = getEnvString("TINALUX_LOG_DEBUG_OUTPUT")) {
        if (const auto parsed = parseBool(*value)) {
            config.enableDebugOutput = *parsed;
        }
    }

    if (const auto value = getEnvString("TINALUX_LOG_ROTATE")) {
        if (const auto parsed = parseBool(*value)) {
            config.rotateFile = *parsed;
        }
    }

    if (const auto value = getEnvString("TINALUX_LOG_MAX_FILE_SIZE")) {
        if (const auto parsed = parseSize(*value)) {
            config.maxFileSize = *parsed;
        }
    }

    if (const auto value = getEnvString("TINALUX_LOG_MAX_FILES")) {
        if (const auto parsed = parseSize(*value)) {
            config.maxFiles = *parsed;
        }
    }

    return config;
}

std::shared_ptr<spdlog::logger> createLogger(const LogConfig& config)
{
    std::vector<spdlog::sink_ptr> sinks;

    if (config.enableConsole) {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sink->set_pattern(config.consolePattern);
        sinks.push_back(std::move(sink));
    }

#if defined(_WIN32) && defined(_MSC_VER)
    if (config.enableDebugOutput) {
        auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        sink->set_pattern(config.plainPattern);
        sinks.push_back(std::move(sink));
    }
#endif

    if (config.enableFile && !config.filePath.empty()) {
        const std::filesystem::path filePath(config.filePath);
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }

        spdlog::sink_ptr sink;
        if (config.rotateFile && config.maxFileSize > 0 && config.maxFiles > 1) {
            sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                config.filePath,
                config.maxFileSize,
                config.maxFiles,
                true);
        } else {
            sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                config.filePath,
                true);
        }
        sink->set_pattern(config.plainPattern);
        sinks.push_back(std::move(sink));
    }

    if (sinks.empty()) {
        sinks.push_back(std::make_shared<spdlog::sinks::null_sink_mt>());
    }

    auto logger = std::make_shared<spdlog::logger>(
        config.loggerName,
        sinks.begin(),
        sinks.end());
    logger->set_level(toSpdlogLevel(config.level));
    logger->flush_on(toSpdlogLevel(config.flushLevel));
    return logger;
}

spdlog::logger& ensureLoggerLocked(LoggerState& state)
{
    if (state.logger == nullptr) {
        state.config = applyEnvironmentOverrides(LogConfig {});
        try {
            state.logger = createLogger(state.config);
            state.initialized = true;
        } catch (...) {
            auto nullSink = std::make_shared<spdlog::sinks::null_sink_mt>();
            state.logger = std::make_shared<spdlog::logger>(
                state.config.loggerName,
                nullSink);
            state.initialized = true;
        }
    }

    return *state.logger;
}

}  // namespace

void initLog(const LogConfig& config)
{
    LoggerState& state = loggerState();
    std::lock_guard<std::mutex> lock(state.mutex);

    state.config = applyEnvironmentOverrides(config);
    try {
        state.logger = createLogger(state.config);
    } catch (...) {
        LogConfig fallback = state.config;
        fallback.enableFile = false;
        fallback.enableConsole = true;
        try {
            state.logger = createLogger(fallback);
            state.config = fallback;
        } catch (...) {
            auto nullSink = std::make_shared<spdlog::sinks::null_sink_mt>();
            state.logger = std::make_shared<spdlog::logger>(
                config.loggerName,
                nullSink);
        }
    }

    state.initialized = true;
}

void shutdownLog()
{
    LoggerState& state = loggerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.logger != nullptr) {
        state.logger->flush();
    }
    state.logger.reset();
    state.initialized = false;
}

bool isLogInitialized()
{
    LoggerState& state = loggerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.initialized;
}

void setLogLevel(LogLevel level)
{
    LoggerState& state = loggerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    ensureLoggerLocked(state).set_level(toSpdlogLevel(level));
}

LogLevel logLevel()
{
    LoggerState& state = loggerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return fromSpdlogLevel(ensureLoggerLocked(state).level());
}

void flushLog()
{
    LoggerState& state = loggerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    ensureLoggerLocked(state).flush();
}

void logMessage(LogLevel level, std::string_view message)
{
    logMessage(level, std::string_view {}, message);
}

void logMessage(LogLevel level, std::string_view category, std::string_view message)
{
    LoggerState& state = loggerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (!category.empty()) {
        ensureLoggerLocked(state).log(
            toSpdlogLevel(level),
            "[{}] {}",
            category,
            message);
        return;
    }

    ensureLoggerLocked(state).log(
        toSpdlogLevel(level),
        spdlog::string_view_t(message.data(), message.size()));
}

}  // namespace tinalux::core
