#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <sys/epoll.h>

// Static helper class for error logging
class Logger
{
public:
	enum LogLevel
	{
		DEBUG = 0,
		INFO = 1,
		WARNING = 2,
		ERROR = 3,
		CRITICAL = 4
	};

private:
	Logger();
	Logger(Logger const &src);
	~Logger();
	Logger &operator=(Logger const &rhs);

	static std::string getCurrentTime()
	{
		time_t now = time(0);
		char buf[80];
		strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
		return std::string(buf);
	}

public:
	static void log(LogLevel level, const std::stringstream &ss, const std::string &filename = "webserv.log")
	{
		log(level, ss.str(), filename);
	}

	static void log(LogLevel level, const std::string &message, const std::string &filename = "webserv.log")
	{
		std::string time = getCurrentTime();
		std::ofstream logFile(filename.c_str(), std::ios::out | std::ios::app);
		if (!logFile.is_open())
		{
			std::cerr << "Error: Log file failed to open" << std::endl;
			return;
		}
		logFile << "[ " << time << " ]";
		std::cerr << "[ " << time << " ]";
		switch (level)
		{
		case DEBUG:
			logFile << " [DEBUG] ";
			std::cerr << " [DEBUG] ";
			break;
		case INFO:
			logFile << " [INFO] ";
			std::cout << " [INFO] ";
			break;
		case WARNING:
			logFile << " [WARNING] ";
			std::cout << " [WARNING] ";
			break;
		case ERROR:
			logFile << " [ERROR] ";
			std::cerr << " [ERROR] ";
			break;
		case CRITICAL:
			logFile << "[CRITICAL] ";
			std::cerr << " [CRITICAL] ";
			break;
		}
		logFile << message << std::endl;
		std::cerr << message << std::endl;
	}
};

#endif /* *********************************************************** ERROR_H */
