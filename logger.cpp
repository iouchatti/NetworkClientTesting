#include "Logger.h"

void Logger::log(const std::string& message) {
    logToFile(message);
    logToConsole(message);
}

void Logger::logToFile(const std::string& message) {
    try {
        static std::ofstream logFile("log.txt", std::ios_base::app | std::ios_base::out);
        auto t = std::time(nullptr);
        std::tm tm = {};
        localtime_s(&tm, &t);
        logFile << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " - " << message << "\n";
        logFile.flush();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception logToFile: " << e.what() << std::endl;
    }
}

void Logger::logToConsole(const std::string& message) {
    std::cout << message << std::endl;
}
