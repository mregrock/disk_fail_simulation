#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <deque>

namespace arctic {

class Logger {
public:
    static void Init(const std::string& filename = "debug.log", size_t maxLines = 300);
    static void Log(const std::string& message);
    static void LogError(const std::string& message);
    static void LogDebug(const std::string& message);

private:
    static void WriteToLog(const std::string& message);
    static void TrimLog();
    
    static std::ofstream file;
    static bool initialized;
    static std::deque<std::string> logLines;
    static size_t maxLogLines;
    static std::string logFilename;
};

#define LOG(msg) Logger::Log(msg)
#define LOG_ERROR(msg) Logger::LogError(msg)
#define LOG_DEBUG(msg) Logger::LogDebug(msg)

} // namespace arctic 