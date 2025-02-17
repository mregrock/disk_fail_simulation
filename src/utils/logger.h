#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <deque>
#include <mutex>

namespace arctic {

class Logger {
public:
    enum class OutputMode {
        FILE_ONLY,
        CONSOLE_ONLY,
        BOTH
    };

    static void Init(const std::string& filename = "debug.log", size_t maxLines = 1000, OutputMode mode = OutputMode::BOTH);
    static void Log(const std::string& message);
    static void LogError(const std::string& message);
    static void LogDebug(const std::string& message);
    static void LogWarning(const std::string& message);
    static void SetOutputMode(OutputMode mode);

private:
    static void WriteToLog(const std::string& message);
    static void TrimLogFile();

    static std::ofstream file;
    static bool initialized;
    static size_t maxLogLines;
    static std::string logFilename;
    static OutputMode outputMode;
    static std::mutex logMutex;
    static size_t logCallsSinceLastTrim;
    static const size_t kTrimCheckFrequency = 1000;
};

#define LOG(msg) Logger::Log(msg)
#define LOG_ERROR(msg) Logger::LogError(msg)
#define LOG_DEBUG(msg) Logger::LogDebug(msg)
#define LOG_WARNING(msg) Logger::LogWarning(msg)
} // namespace arctic 
