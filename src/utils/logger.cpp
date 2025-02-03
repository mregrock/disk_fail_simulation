#include "logger.h"
#include <ctime>
#include <sstream>
#include <fstream>

namespace arctic {

std::ofstream Logger::file;
bool Logger::initialized = false;
std::deque<std::string> Logger::logLines;
size_t Logger::maxLogLines = 300;
std::string Logger::logFilename;

void Logger::Init(const std::string& filename, size_t maxLines) {
    if (!initialized) {
        maxLogLines = maxLines;
        logFilename = filename;
        file.open(filename, std::ios::out | std::ios::trunc);
        initialized = true;
        
        time_t now = time(0);
        std::string datetime = ctime(&now);
        WriteToLog("\n=== Log started at " + datetime + "===\n");
    }
}

void Logger::WriteToLog(const std::string& message) {
    logLines.push_back(message);
    if (logLines.size() > maxLogLines) {
        logLines.pop_front();
    }
    
    if (file.is_open()) {
        file.close();
    }
    file.open(logFilename, std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        for (const auto& line : logLines) {
            file << line;
        }
        file.flush();
    }
}

void Logger::Log(const std::string& message) {
    if (!initialized) Init();
    
    time_t now = time(0);
    std::string datetime = ctime(&now);
    datetime = datetime.substr(0, datetime.length()-1);
    
    std::stringstream ss;
    ss << "[" << datetime << "] INFO: " << message << std::endl;
    
    WriteToLog(ss.str());
    std::cout << ss.str();
}

void Logger::LogError(const std::string& message) {
    if (!initialized) Init();
    
    time_t now = time(0);
    std::string datetime = ctime(&now);
    datetime = datetime.substr(0, datetime.length()-1);
    
    std::stringstream ss;
    ss << "[" << datetime << "] ERROR: " << message << std::endl;
    
    WriteToLog(ss.str());
    std::cout << "\033[1;31m" << ss.str() << "\033[0m";
}

void Logger::LogDebug(const std::string& message) {
    if (!initialized) Init();
    
    time_t now = time(0);
    std::string datetime = ctime(&now);
    datetime = datetime.substr(0, datetime.length()-1);
    
    std::stringstream ss;
    ss << "[" << datetime << "] DEBUG: " << message << std::endl;
    
    WriteToLog(ss.str());
    std::cout << "\033[1;34m" << ss.str() << "\033[0m";
}

} // namespace arctic 