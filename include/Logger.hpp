#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR_LEVEL,
    DEBUG
};

class Logger {
private:
    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    static std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::INFO:         return "INFO";
            case LogLevel::WARNING:      return "WARN";
            case LogLevel::ERROR_LEVEL:  return "ERROR";
            case LogLevel::DEBUG:        return "DEBUG";
        }
        return "UNKNOWN";
    }

public:
    static void log(LogLevel level, const std::string& message) {
        std::cout << "[" << getCurrentTimestamp() << "] "
                  << "[" << levelToString(level) << "] "
                  << message << std::endl;
    }
};

#define LOG_INFO(msg) Logger::log(LogLevel::INFO, msg)
#define LOG_WARN(msg) Logger::log(LogLevel::WARNING, msg)
#define LOG_ERROR(msg) Logger::log(LogLevel::ERROR_LEVEL, msg)
#define LOG_DEBUG(msg) Logger::log(LogLevel::DEBUG, msg)

#endif