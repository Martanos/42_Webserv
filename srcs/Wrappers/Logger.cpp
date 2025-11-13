#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include <iomanip>
#include <iostream>

// USAGE CONVENTIONS:
//  - Always use enhanced logging methods: debug/info/warning/error/critical
//    with (message, __FILE__, __LINE__, __PRETTY_FUNCTION__) for every log call.
//  - ACCESS logs (Logger::access) should be used for request/response events and always include file/line/function.
//  - Do NOT use removed single-argument overloads; all log calls must provide full source context.
//  - Example:
//      Logger::debug("Some debug info", __FILE__, __LINE__, __PRETTY_FUNCTION__);
//      Logger::access("Request log entry", __FILE__, __LINE__, __PRETTY_FUNCTION__);
//  - Compile-time filtering is controlled by LOG_MIN_LEVEL macro (see Makefile):
//      - Default: WARNING and above
//      - Debug build: DEBUG and above
//  - All log entries are written to a single session log file in the logs/ directory.

// Static member initialization
std::string Logger::_sessionId = "";
std::string Logger::_logDirectory = "logs";
bool Logger::_sessionInitialized = false;
std::ofstream Logger::_logFile;
// Compile-time configurable minimum log level. Override via -DLOG_MIN_LEVEL=<int> in build.
// 0=DEBUG,1=INFO,2=WARNING,3=ERROR,4=CRITICAL
#ifndef LOG_MIN_LEVEL
#define LOG_MIN_LEVEL 2 // Default to WARNING: show WARNING/ERROR/CRITICAL by default
#endif
Logger::LogLevel Logger::_minLogLevel = static_cast<Logger::LogLevel>(LOG_MIN_LEVEL);

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Logger::Logger()
{
	// Non-instantiable
}

Logger::Logger(Logger const &src)
{
	(void)src;
	// Non-instantiable
}

Logger::~Logger()
{
	// Non-instantiable
}

Logger &Logger::operator=(Logger const &rhs)
{
	(void)rhs;
	// Non-instantiable
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void Logger::initializeSession(const std::string &logDir)
{
	if (_sessionInitialized)
	{
		closeSession();
	}

	_logDirectory = logDir;

	// Create logs directory if it doesn't exist
	_createLogDirectory();

	// Generate unique session ID with timestamp
	_generateSessionId();

	// Open log file for this session
	_openSessionLogFile();

	_sessionInitialized = true;

	// Log session start
	log(INFO, "=== WebServ Session Started ===");
	log(INFO, "Session ID: " + _sessionId);
	log(INFO, "Log Directory: " + _logDirectory);
	log(INFO, "Process ID: " + _getProcessId());
}

void Logger::closeSession()
{
	if (_sessionInitialized && _logFile.is_open())
	{
		log(INFO, "=== WebServ Session Ended ===");
		log(INFO, "Session ID: " + _sessionId);
		_logFile.close();
		_sessionInitialized = false;
	}
}

void Logger::setMinLogLevel(LogLevel level)
{
	_minLogLevel = level;
}

void Logger::log(LogLevel level, const std::string &message)
{
	// Apply level filtering (DEBUG suppressed in normal builds)
	if (level < _minLogLevel)
		return;

	std::string timestamp = _getCurrentTime();
	std::string levelStr = _getLevelString(level);
	std::string logEntry = "[" + timestamp + "] " + levelStr + " " + message;

	// Log to session file
	if (_sessionInitialized && _logFile.is_open())
	{
		_logFile << logEntry << std::endl;
		_logFile.flush(); // Ensure immediate write
	}

	// Log to console with color coding
	_logToConsole(level, logEntry);
}

void Logger::log(LogLevel level, const std::stringstream &ss)
{
	log(level, ss.str());
}

void Logger::log(LogLevel level, const std::string &message, const std::string &file, int line,
				 const std::string &function)
{
	if (level < _minLogLevel)
		return;
	// Extract just the filename from the full path
	std::string filename = file;
	size_t lastSlash = filename.find_last_of("/\\");
	if (lastSlash != std::string::npos)
	{
		filename = filename.substr(lastSlash + 1);
	}

	std::string timestamp = _getCurrentTime();
	std::string levelStr = _getLevelString(level);
	std::string logEntry = "[" + timestamp + "] " + levelStr + " [" + filename + ":" + StrUtils::toString<int>(line) +
						   "] " + function + " " + message;

	// Log to session file
	if (_sessionInitialized && _logFile.is_open())
	{
		_logFile << logEntry << std::endl;
		_logFile.flush(); // Ensure immediate write
	}

	// Log to console with color coding
	_logToConsole(level, logEntry);
}

// Removed message-only convenience overloads to enforce consistent style (file/line/function).

void Logger::access(const std::string &message, const std::string &file, int line, const std::string &function)
{
	// Access logs bypass level filtering but retain source context for traceability
	std::string filename = file;
	size_t lastSlash = filename.find_last_of("/\\");
	if (lastSlash != std::string::npos)
		filename = filename.substr(lastSlash + 1);
	std::string timestamp = _getCurrentTime();
	std::stringstream ss;
	ss << "[" << timestamp << "] [ACCESS]  [" << filename << ":" << line << "] " << function << " " << message;
	std::string logEntry = ss.str();
	if (_sessionInitialized && _logFile.is_open())
	{
		_logFile << logEntry << std::endl;
		_logFile.flush();
	}
	_logToConsole(INFO, logEntry);
}

void Logger::debug(const std::string &message, const std::string &file, int line, const std::string &function)
{
	log(DEBUG, message, file, line, function);
}

void Logger::info(const std::string &message, const std::string &file, int line, const std::string &function)
{
	log(INFO, message, file, line, function);
}

void Logger::warning(const std::string &message, const std::string &file, int line, const std::string &function)
{
	log(WARNING, message, file, line, function);
}

void Logger::error(const std::string &message, const std::string &file, int line, const std::string &function)
{
	log(ERROR, message, file, line, function);
}

void Logger::critical(const std::string &message, const std::string &file, int line, const std::string &function)
{
	log(CRITICAL, message, file, line, function);
}

void Logger::logRequest(const std::string &method, const std::string &uri, const std::string &clientIP, int statusCode)
{
	std::stringstream ss;
	ss << clientIP << " - \"" << method << " " << uri << "\" " << statusCode;
	// Route through enhanced access API; use this file/line/function
	Logger::access(ss.str(), __FILE__, __LINE__, __PRETTY_FUNCTION__);
}

void Logger::logErrno(LogLevel level, const std::string &message)
{
	std::stringstream ss;
	ss << message << ": " << std::strerror(errno);
	log(level, ss.str());
}

void Logger::logErrno(LogLevel level, const std::string &message, const std::string &file, int line)
{
	std::stringstream ss;
	ss << message << ": " << std::strerror(errno);
	log(level, ss.str(), file, line, __FUNCTION__);
}

void Logger::logServerStart(const std::string &host, int port)
{
	std::stringstream ss;
	ss << "Server started on " << host << ":" << port;
	log(INFO, ss.str());
}

void Logger::logClientConnect(const std::string &clientIP, int port)
{
	std::stringstream ss;
	ss << "Client connected from " << clientIP << ":" << port;
	log(INFO, ss.str());
}

void Logger::logClientDisconnect(const std::string &clientIP, int port)
{
	std::stringstream ss;
	ss << "Client disconnected from " << clientIP << ":" << port;
	log(INFO, ss.str());
}

void Logger::logCGIExecution(const std::string &scriptPath, int statusCode, const std::string &error)
{
	std::stringstream ss;
	ss << "CGI execution: " << scriptPath << " (Status: " << statusCode << ")";
	if (!error.empty())
		ss << " Error: " << error;
	log(INFO, ss.str());
}

/*
** ---------------------------- PRIVATE METHODS -------------------------------
*/

std::string Logger::_getCurrentTime()
{
	std::time_t now = std::time(0);
	char buf[80];
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
	return std::string(buf);
}

std::string Logger::_getLevelString(LogLevel level)
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

void Logger::_logToConsole(LogLevel level, const std::string &logEntry)
{
	std::ostream &out = (level >= ERROR) ? std::cerr : std::cout;

	// Add color coding for different log levels
	if (level >= ERROR)
		out << "\033[31m"; // Red for errors
	else if (level == WARNING)
		out << "\033[33m"; // Yellow for warnings
	else if (level == INFO)
		out << "\033[32m"; // Green for info
	else if (level == DEBUG)
		out << "\033[36m"; // Cyan for debug

	out << logEntry << "\033[0m" << std::endl; // Reset color
}

void Logger::_createLogDirectory()
{
	struct stat st;
	if (stat(_logDirectory.c_str(), &st) != 0)
	{
		if (mkdir(_logDirectory.c_str(), 0755) != 0)
		{
			std::cerr << "Failed to create log directory: " << _logDirectory << std::endl;
			_logDirectory = "."; // Fallback to current directory
		}
	}
}

void Logger::_generateSessionId()
{
	std::time_t now = std::time(0);
	struct tm *timeinfo = std::localtime(&now);

	std::stringstream ss;
	ss << "webserv_" << (1900 + timeinfo->tm_year) << std::setfill('0') << std::setw(2) << (timeinfo->tm_mon + 1)
	   << std::setfill('0') << std::setw(2) << timeinfo->tm_mday << "_" << std::setfill('0') << std::setw(2)
	   << timeinfo->tm_hour << std::setfill('0') << std::setw(2) << timeinfo->tm_min << std::setfill('0')
	   << std::setw(2) << timeinfo->tm_sec << "_" << getpid();

	_sessionId = ss.str();
}

void Logger::_openSessionLogFile()
{
	std::string logFilePath = _logDirectory + "/" + _sessionId + ".log";
	_logFile.open(logFilePath.c_str(), std::ios::out | std::ios::app);

	if (!_logFile.is_open())
	{
		std::cerr << "Failed to open log file: " << logFilePath << std::endl;
		// Fallback to stdout
		_logFile.open("/dev/stdout");
	}
}

std::string Logger::_getProcessId()
{
	std::stringstream ss;
	ss << getpid();
	return ss.str();
}
