#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <string>
#include <sys/epoll.h>
#include <cstring>

// Enhanced static logger class with better formatting
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
	// Non-instantiable
	Logger();
	Logger(Logger const &src);
	~Logger();
	Logger &operator=(Logger const &rhs);

	static std::string _getCurrentTime()
	{
		std::time_t now = std::time(0);
		char buf[80];
		std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
		return std::string(buf);
	}

	static std::string _getLevelString(LogLevel level)
	{
		switch (level)
		{
		case DEBUG:
			return "[DEBUG]   ";
		case INFO:
			return "[INFO]    ";
		case WARNING:
			return "[WARNING] ";
		case ERROR:
			return "[ERROR]   ";
		case CRITICAL:
			return "[CRITICAL]";
		default:
			return "[UNKNOWN] ";
		}
	}

	static std::ostream &_getOutputStream(LogLevel level)
	{
		return (level >= ERROR) ? std::cerr : std::cout;
	}

public:
	// Primary logging method
	static void log(LogLevel level, const std::string &message, const std::string &filename = "webserv.log")
	{
		std::string timestamp = _getCurrentTime();
		std::string levelStr = _getLevelString(level);

		// Log to file
		std::ofstream logFile(filename.c_str(), std::ios::out | std::ios::app);
		if (logFile.is_open())
		{
			logFile << "[" << timestamp << "] " << levelStr << " " << message << std::endl;
			logFile.close();
		}

		// Log to console
		std::ostream &out = _getOutputStream(level);
		out << "[" << timestamp << "] " << levelStr << " " << message << std::endl;
	}

	// Convenience method for stringstream
	static void log(LogLevel level, const std::stringstream &ss, const std::string &filename = "webserv.log")
	{
		log(level, ss.str(), filename);
	}

	// Convenience methods for different levels
	static void debug(const std::string &message) { log(DEBUG, message); }
	static void info(const std::string &message) { log(INFO, message); }
	static void warning(const std::string &message) { log(WARNING, message); }
	static void error(const std::string &message) { log(ERROR, message); }
	static void critical(const std::string &message) { log(CRITICAL, message); }

	// HTTP request logging
	static void logRequest(const std::string &method, const std::string &uri,
						   const std::string &clientIP, int statusCode)
	{
		std::stringstream ss;
		ss << clientIP << " - \"" << method << " " << uri << "\" " << statusCode;
		log(INFO, ss.str());
	}

	// Error with errno
	static void logErrno(LogLevel level, const std::string &message)
	{
		std::stringstream ss;
		ss << message << ": " << std::strerror(errno);
		log(level, ss.str());
	}
};

#endif /* LOGGER_HPP */
