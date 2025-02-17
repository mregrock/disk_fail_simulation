#include "logger.h"
#include <ctime>
#include <sstream>
#include <fstream>
#include <mutex>
#include <deque>
#include <vector>
#include <algorithm>

namespace arctic {

std::ofstream Logger::file;
bool Logger::initialized = false;
size_t Logger::maxLogLines = 1000;
std::string Logger::logFilename;
Logger::OutputMode Logger::outputMode = Logger::OutputMode::BOTH;
std::mutex Logger::logMutex;
size_t Logger::logCallsSinceLastTrim = 0;

void Logger::Init(const std::string& filename, size_t maxLines /*= 300 (параметр больше не используется)*/, OutputMode mode /*= OutputMode::BOTH*/) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (!initialized) {
        maxLogLines = maxLines;
        logFilename = filename;
        outputMode = mode;
        logCallsSinceLastTrim = 0;

        if (outputMode != OutputMode::CONSOLE_ONLY && !logFilename.empty()) {
            file.open(logFilename, std::ios::out | std::ios::app);
            if (!file.is_open()) {
                std::cerr << "ERROR: Could not open log file for appending: " << logFilename << std::endl;
            }
        }

        initialized = true;

        time_t now = time(0);
        std::string datetime = ctime(&now);
        datetime = datetime.substr(0, datetime.length()-1);
        if (outputMode != OutputMode::CONSOLE_ONLY && file.is_open()) {
             file << "\n=== Log started at " << datetime << " ===" << std::endl;
        }
        if (outputMode != OutputMode::FILE_ONLY) {
             std::cout << "\n=== Log started at " << datetime << " ===" << std::endl;
        }
    }
}

void Logger::SetOutputMode(OutputMode mode) {
    std::lock_guard<std::mutex> lock(logMutex);
    outputMode = mode;
}

void Logger::WriteToLog(const std::string& message) {
    if (outputMode != OutputMode::CONSOLE_ONLY && file.is_open()) {
        file << message;
        file.flush();
    }
}

void Logger::TrimLogFile() {
    if (outputMode == OutputMode::CONSOLE_ONLY || logFilename.empty() || maxLogLines == 0) {
        return;
    }

    if (file.is_open()) {
        file.close();
    }

    std::deque<std::string> lines;
    std::ifstream infile(logFilename);
    std::string line;
    size_t lineCount = 0;

    if (infile.is_open()) {
        while (std::getline(infile, line)) {
            lines.push_back(line);
            lineCount++;
            if (lines.size() > maxLogLines) {
                lines.pop_front();
            }
        }
        infile.close();
    } else {
        std::cerr << "ERROR: Could not open log file for trimming: " << logFilename << std::endl;
        file.open(logFilename, std::ios::out | std::ios::app);
        return;
    }

    file.open(logFilename, std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        for (const auto& l : lines) {
            file << l << std::endl;
        }
        file.close();
    } else {
         std::cerr << "ERROR: Could not open log file for writing trimmed content: " << logFilename << std::endl;
    }

    file.open(logFilename, std::ios::out | std::ios::app);
     if (!file.is_open()) {
          std::cerr << "ERROR: Could not reopen log file for appending after trimming: " << logFilename << std::endl;
     }
}

void Logger::Log(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (!initialized) Init();

    time_t now = time(0);
    std::string datetime = ctime(&now);
    datetime = datetime.substr(0, datetime.length()-1);
    std::stringstream ss;
    ss << "[" << datetime << "] INFO: " << message << std::endl;
    std::string logMessage = ss.str();

    if (outputMode != OutputMode::CONSOLE_ONLY) {
        WriteToLog(logMessage);
    }
    if (outputMode != OutputMode::FILE_ONLY) {
        std::cout << logMessage;
    }

    logCallsSinceLastTrim++;
    if (logCallsSinceLastTrim >= kTrimCheckFrequency) {
        TrimLogFile();
        logCallsSinceLastTrim = 0;
    }
}

void Logger::LogError(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (!initialized) Init();

    time_t now = time(0);
    std::string datetime = ctime(&now);
    datetime = datetime.substr(0, datetime.length()-1);
    std::stringstream ss;
    ss << "[" << datetime << "] ERROR: " << message << std::endl;
    std::string logMessage = ss.str();

    if (outputMode != OutputMode::CONSOLE_ONLY) {
        WriteToLog(logMessage);
    }
    if (outputMode != OutputMode::FILE_ONLY) {
        std::cerr << "\033[1;31m" << logMessage << "\033[0m";
    }

    logCallsSinceLastTrim++;
    if (logCallsSinceLastTrim >= kTrimCheckFrequency) {
        TrimLogFile();
        logCallsSinceLastTrim = 0;
    }
}

void Logger::LogDebug(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (!initialized) Init();

    time_t now = time(0);
    std::string datetime = ctime(&now);
    datetime = datetime.substr(0, datetime.length()-1);
    std::stringstream ss;
    ss << "[" << datetime << "] DEBUG: " << message << std::endl;
    std::string logMessage = ss.str();

    if (outputMode != OutputMode::CONSOLE_ONLY) {
        WriteToLog(logMessage);
    }
    if (outputMode != OutputMode::FILE_ONLY) {
        std::cout << "\033[1;34m" << logMessage << "\033[0m";
    }

    logCallsSinceLastTrim++;
    if (logCallsSinceLastTrim >= kTrimCheckFrequency) {
        TrimLogFile();
        logCallsSinceLastTrim = 0;
    }
}

void Logger::LogWarning(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (!initialized) Init();

    time_t now = time(0);
    std::string datetime = ctime(&now);
    datetime = datetime.substr(0, datetime.length()-1);
    std::stringstream ss;
    ss << "[" << datetime << "] WARNING: " << message << std::endl;
    std::string logMessage = ss.str();

    if (outputMode != OutputMode::CONSOLE_ONLY) {
        WriteToLog(logMessage);
    }
    if (outputMode != OutputMode::FILE_ONLY) {
        std::cerr << "\033[1;33m" << logMessage << "\033[0m";
    }

    logCallsSinceLastTrim++;
    if (logCallsSinceLastTrim >= kTrimCheckFrequency) {
        TrimLogFile();
        logCallsSinceLastTrim = 0;
    }
}

} // namespace arctic 