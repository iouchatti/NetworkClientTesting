#pragma once

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <mutex>

class Logger {
public:
    static void log(const std::string& message);

private:
    static void logToFile(const std::string& message);
    static void logToConsole(const std::string& message);
};
