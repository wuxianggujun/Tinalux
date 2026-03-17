#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <spdlog/fmt/fmt.h>

namespace tinalux::core {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off,
};

struct LogConfig {
    std::string loggerName = "tinalux";
    std::string androidTag = "Tinalux";
    std::string filePath = "logs/tinalux.log";
    std::string consolePattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v";
    std::string plainPattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] %v";
#if defined(__ANDROID__)
    bool enableConsole = false;
    bool enableAndroidLog = true;
#else
    bool enableConsole = true;
    bool enableAndroidLog = false;
#endif
    bool enableFile = true;
    bool enableDebugOutput = true;
    bool rotateFile = true;
    std::size_t maxFileSize = 5 * 1024 * 1024;
    std::size_t maxFiles = 3;
    LogLevel level = LogLevel::Info;
    LogLevel flushLevel = LogLevel::Error;
};

void initLog(const LogConfig& config = {});
void shutdownLog();
bool isLogInitialized();
void setLogLevel(LogLevel level);
LogLevel logLevel();
void flushLog();
void logMessage(LogLevel level, std::string_view message);
void logMessage(LogLevel level, std::string_view category, std::string_view message);

template <typename... Args>
void log(LogLevel level, fmt::format_string<Args...> format, Args&&... args)
{
    logMessage(level, fmt::format(format, std::forward<Args>(args)...));
}

template <typename... Args>
void logCat(
    LogLevel level,
    std::string_view category,
    fmt::format_string<Args...> format,
    Args&&... args)
{
    logMessage(level, category, fmt::format(format, std::forward<Args>(args)...));
}

template <typename... Args>
void logTrace(fmt::format_string<Args...> format, Args&&... args)
{
    log(LogLevel::Trace, format, std::forward<Args>(args)...);
}

template <typename... Args>
void logDebug(fmt::format_string<Args...> format, Args&&... args)
{
    log(LogLevel::Debug, format, std::forward<Args>(args)...);
}

template <typename... Args>
void logInfo(fmt::format_string<Args...> format, Args&&... args)
{
    log(LogLevel::Info, format, std::forward<Args>(args)...);
}

template <typename... Args>
void logWarn(fmt::format_string<Args...> format, Args&&... args)
{
    log(LogLevel::Warn, format, std::forward<Args>(args)...);
}

template <typename... Args>
void logError(fmt::format_string<Args...> format, Args&&... args)
{
    log(LogLevel::Error, format, std::forward<Args>(args)...);
}

template <typename... Args>
void logCritical(fmt::format_string<Args...> format, Args&&... args)
{
    log(LogLevel::Critical, format, std::forward<Args>(args)...);
}

template <typename... Args>
void logTraceCat(std::string_view category, fmt::format_string<Args...> format, Args&&... args)
{
    logCat(LogLevel::Trace, category, format, std::forward<Args>(args)...);
}

template <typename... Args>
void logDebugCat(std::string_view category, fmt::format_string<Args...> format, Args&&... args)
{
    logCat(LogLevel::Debug, category, format, std::forward<Args>(args)...);
}

template <typename... Args>
void logInfoCat(std::string_view category, fmt::format_string<Args...> format, Args&&... args)
{
    logCat(LogLevel::Info, category, format, std::forward<Args>(args)...);
}

template <typename... Args>
void logWarnCat(std::string_view category, fmt::format_string<Args...> format, Args&&... args)
{
    logCat(LogLevel::Warn, category, format, std::forward<Args>(args)...);
}

template <typename... Args>
void logErrorCat(std::string_view category, fmt::format_string<Args...> format, Args&&... args)
{
    logCat(LogLevel::Error, category, format, std::forward<Args>(args)...);
}

template <typename... Args>
void logCriticalCat(std::string_view category, fmt::format_string<Args...> format, Args&&... args)
{
    logCat(LogLevel::Critical, category, format, std::forward<Args>(args)...);
}

}  // namespace tinalux::core

#define TINALUX_LOG_TRACE(...) ::tinalux::core::logTrace(__VA_ARGS__)
#define TINALUX_LOG_DEBUG(...) ::tinalux::core::logDebug(__VA_ARGS__)
#define TINALUX_LOG_INFO(...) ::tinalux::core::logInfo(__VA_ARGS__)
#define TINALUX_LOG_WARN(...) ::tinalux::core::logWarn(__VA_ARGS__)
#define TINALUX_LOG_ERROR(...) ::tinalux::core::logError(__VA_ARGS__)
#define TINALUX_LOG_CRITICAL(...) ::tinalux::core::logCritical(__VA_ARGS__)
#define TINALUX_LOG_TRACE_CAT(category, ...) ::tinalux::core::logTraceCat(category, __VA_ARGS__)
#define TINALUX_LOG_DEBUG_CAT(category, ...) ::tinalux::core::logDebugCat(category, __VA_ARGS__)
#define TINALUX_LOG_INFO_CAT(category, ...) ::tinalux::core::logInfoCat(category, __VA_ARGS__)
#define TINALUX_LOG_WARN_CAT(category, ...) ::tinalux::core::logWarnCat(category, __VA_ARGS__)
#define TINALUX_LOG_ERROR_CAT(category, ...) ::tinalux::core::logErrorCat(category, __VA_ARGS__)
#define TINALUX_LOG_CRITICAL_CAT(category, ...) ::tinalux::core::logCriticalCat(category, __VA_ARGS__)
