#include "includes/Logger.hpp"
#include <iostream>
#include <unistd.h>

int main()
{
    std::cout << "Testing Enhanced Logger with Session Management" << std::endl;
    
    // Initialize logger session
    Logger::initializeSession("test_logs");
    
    // Test different log levels
    Logger::info("This is an info message");
    Logger::warning("This is a warning message");
    Logger::error("This is an error message");
    Logger::debug("This is a debug message");
    
    // Test specialized logging methods
    Logger::logServerStart("localhost", 8080);
    Logger::logClientConnect("192.168.1.100", 54321);
    Logger::logRequest("GET", "/index.html", "192.168.1.100", 200);
    Logger::logCGIExecution("/usr/bin/php-cgi", 0, "");
    Logger::logClientDisconnect("192.168.1.100", 54321);
    
    // Test stringstream logging
    std::stringstream ss;
    ss << "Stringstream test: " << 42 << " items processed";
    Logger::log(Logger::INFO, ss);
    
    // Test errno logging
    Logger::logErrno(Logger::ERROR, "Failed to open file");
    
    std::cout << "Session ID: " << Logger::getSessionId() << std::endl;
    std::cout << "Log Directory: " << Logger::getLogDirectory() << std::endl;
    
    // Close session
    Logger::closeSession();
    
    std::cout << "Logger test completed. Check the test_logs directory for session log files." << std::endl;
    
    return 0;
}