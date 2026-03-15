#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "tinalux/core/Log.h"

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void setEnvValue(const char* name, const std::string& value)
{
#if defined(_WIN32)
    _putenv_s(name, value.c_str());
#else
    setenv(name, value.c_str(), 1);
#endif
}

void clearEnvValue(const char* name)
{
#if defined(_WIN32)
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif
}

}  // namespace

int main()
{
    namespace fs = std::filesystem;
    using namespace tinalux::core;

    const fs::path logPath = fs::current_path() / "tinalux-log-smoke.log";
    std::error_code ec;
    fs::remove(logPath, ec);

    LogConfig config;
    config.loggerName = "tinalux-log-smoke";
    config.filePath = logPath.string();
    config.enableConsole = false;
    config.enableFile = true;
    config.enableDebugOutput = false;
    config.level = LogLevel::Trace;
    config.flushLevel = LogLevel::Trace;
    config.plainPattern = "%v";

    initLog(config);
    expect(isLogInitialized(), "logger should be initialized");
    expect(logLevel() == LogLevel::Trace, "logger level should match initialized level");

    setLogLevel(LogLevel::Debug);
    expect(logLevel() == LogLevel::Debug, "setLogLevel should update current level");

    logInfo("log smoke {}", 42);
    flushLog();

    expect(fs::exists(logPath), "log file should be created");
    expect(fs::file_size(logPath) > 0, "log file should contain output");

    std::ifstream input(logPath);
    const std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    expect(content.find("log smoke 42") != std::string::npos, "log file should contain formatted message");
    expect(content.find("[info]") == std::string::npos, "custom plain pattern should remove default prefix");

    const fs::path rotatePath = fs::current_path() / "tinalux-log-rotate.log";
    const fs::path rotatePath1 = fs::current_path() / "tinalux-log-rotate.1.log";
    fs::remove(rotatePath, ec);
    fs::remove(rotatePath1, ec);

    LogConfig rotateConfig;
    rotateConfig.loggerName = "tinalux-log-rotate";
    rotateConfig.filePath = rotatePath.string();
    rotateConfig.enableConsole = false;
    rotateConfig.enableFile = true;
    rotateConfig.enableDebugOutput = false;
    rotateConfig.rotateFile = true;
    rotateConfig.maxFileSize = 128;
    rotateConfig.maxFiles = 2;
    rotateConfig.level = LogLevel::Trace;
    rotateConfig.flushLevel = LogLevel::Trace;
    rotateConfig.plainPattern = "%v";

    initLog(rotateConfig);
    for (int index = 0; index < 32; ++index) {
        logInfoCat("smoke", "rotation message {:02d} {}", index, "xxxxxxxxxxxxxxxxxxxxxxxx");
    }
    flushLog();
    expect(fs::exists(rotatePath), "rotating log base file should exist");
    expect(fs::exists(rotatePath1), "rotating log should create rollover file");

    shutdownLog();
    fs::remove(logPath, ec);
    fs::remove(rotatePath, ec);
    fs::remove(rotatePath1, ec);

    const fs::path envLogPath = fs::current_path() / "tinalux-log-env-smoke.log";
    fs::remove(envLogPath, ec);
    const fs::path envRotatePath1 = fs::current_path() / "tinalux-log-env-smoke.1.log";
    fs::remove(envRotatePath1, ec);

    setEnvValue("TINALUX_LOGGER_NAME", "tinalux-log-env-smoke");
    setEnvValue("TINALUX_LOG_FILE_PATH", envLogPath.string());
    setEnvValue("TINALUX_LOG_PLAIN_PATTERN", "%v");
    setEnvValue("TINALUX_LOG_LEVEL", "trace");
    setEnvValue("TINALUX_LOG_FLUSH_LEVEL", "trace");
    setEnvValue("TINALUX_LOG_CONSOLE", "off");
    setEnvValue("TINALUX_LOG_FILE", "on");
    setEnvValue("TINALUX_LOG_DEBUG_OUTPUT", "off");
    setEnvValue("TINALUX_LOG_ROTATE", "on");
    setEnvValue("TINALUX_LOG_MAX_FILE_SIZE", "128");
    setEnvValue("TINALUX_LOG_MAX_FILES", "2");

    LogConfig envConfig;
    envConfig.loggerName = "ignored-by-env";
    envConfig.filePath = "ignored-by-env.log";
    envConfig.enableConsole = true;
    envConfig.enableFile = false;
    envConfig.rotateFile = false;
    envConfig.level = LogLevel::Error;
    envConfig.flushLevel = LogLevel::Critical;
    initLog(envConfig);

    expect(logLevel() == LogLevel::Trace, "environment log level should override init config");
    for (int index = 0; index < 32; ++index) {
        logDebugCat("env", "env override smoke {:02d} {}", index, "yyyyyyyyyyyyyyyyyyyyyyyy");
    }
    flushLog();
    expect(fs::exists(envLogPath), "environment file path should be used");
    expect(fs::file_size(envLogPath) > 0, "environment-configured log file should contain output");
    expect(fs::exists(envRotatePath1), "environment rotation config should create rollover file");
    std::ifstream envInput(envLogPath);
    const std::string envContent((std::istreambuf_iterator<char>(envInput)), std::istreambuf_iterator<char>());
    expect(envContent.find("[env]") != std::string::npos, "category tag should be written into message body");
    expect(envContent.find("[debug]") == std::string::npos, "environment plain pattern should override default prefix");

    shutdownLog();
    fs::remove(envLogPath, ec);
    fs::remove(envRotatePath1, ec);
    clearEnvValue("TINALUX_LOGGER_NAME");
    clearEnvValue("TINALUX_LOG_FILE_PATH");
    clearEnvValue("TINALUX_LOG_PLAIN_PATTERN");
    clearEnvValue("TINALUX_LOG_LEVEL");
    clearEnvValue("TINALUX_LOG_FLUSH_LEVEL");
    clearEnvValue("TINALUX_LOG_CONSOLE");
    clearEnvValue("TINALUX_LOG_FILE");
    clearEnvValue("TINALUX_LOG_DEBUG_OUTPUT");
    clearEnvValue("TINALUX_LOG_ROTATE");
    clearEnvValue("TINALUX_LOG_MAX_FILE_SIZE");
    clearEnvValue("TINALUX_LOG_MAX_FILES");
    return 0;
}
