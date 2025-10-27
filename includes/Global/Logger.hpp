#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <unistd.h>

// Enhanced session-based logger class with better formatting and file management
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

	// Session management
	static std::string _sessionId;
	static std::string _logDirectory;
	static bool _sessionInitialized;
	static std::ofstream _logFile;
	static LogLevel _minLogLevel;

	// Private helper methods
	static std::string _getCurrentTime();
	static std::string _getLevelString(LogLevel level);
	static void _logToConsole(LogLevel level, const std::string &logEntry);
	static void _createLogDirectory();
	static void _generateSessionId();
	static void _openSessionLogFile();
	static std::string _getProcessId();

public:
	// Session management
	static void initializeSession(const std::string &logDir = "logs");
	static void closeSession();
	static void setMinLogLevel(LogLevel level);
	static std::string getSessionId()
	{
		return _sessionId;
	}
	static std::string getLogDirectory()
	{
		return _logDirectory;
	}

	// Primary logging method
	static void log(LogLevel level, const std::string &message);

	// Enhanced logging method with file and line information and function name (use __FILE__, __LINE__, and
	// __PRETTY_FUNCTION__ macros)
	static void log(LogLevel level, const std::string &message, const std::string &file, int line,
					const std::string &function);

	// Convenience method for stringstream
	static void log(LogLevel level, const std::stringstream &ss);

	// Convenience methods for different levels
	static void debug(const std::string &message);
	static void info(const std::string &message);
	static void warning(const std::string &message);
	static void error(const std::string &message);
	static void critical(const std::string &message);

	// Enhanced convenience methods with file and line information
	static void debug(const std::string &message, const std::string &file, int line, const std::string &function);
	static void info(const std::string &message, const std::string &file, int line, const std::string &function);
	static void warning(const std::string &message, const std::string &file, int line, const std::string &function);
	static void error(const std::string &message, const std::string &file, int line, const std::string &function);
	static void critical(const std::string &message, const std::string &file, int line, const std::string &function);

	// Specialized logging methods
	static void logRequest(const std::string &method, const std::string &uri, const std::string &clientIP,
						   int statusCode);
	static void logErrno(LogLevel level, const std::string &message);
	static void logErrno(LogLevel level, const std::string &message, const std::string &file, int line);
	static void logServerStart(const std::string &host, int port);
	static void logClientConnect(const std::string &clientIP, int port);
	static void logClientDisconnect(const std::string &clientIP, int port);
	static void logCGIExecution(const std::string &scriptPath, int statusCode, const std::string &error = "");
};

#endif /* LOGGER_HPP */
