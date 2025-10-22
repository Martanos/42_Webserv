#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include <iomanip>
#include <iostream>

// Static member initialization
std::string Logger::_sessionId = "";
std::string Logger::_logDirectory = "logs";
bool Logger::_sessionInitialized = false;
std::ofstream Logger::_logFile;
Logger::LogLevel Logger::_minLogLevel = Logger::INFO;

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
	// if (level < _minLogLevel)
	// 	return;

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

void Logger::debug(const std::string &message)
{
	log(DEBUG, message);
}

void Logger::info(const std::string &message)
{
	log(INFO, message);
}

void Logger::warning(const std::string &message)
{
	log(WARNING, message);
}

void Logger::error(const std::string &message)
{
	log(ERROR, message);
}

void Logger::critical(const std::string &message)
{
	log(CRITICAL, message);
}

void Logger::debug(const std::string &message, const std::string &file, int line)
{
	log(DEBUG, message, file, line);
}

void Logger::info(const std::string &message, const std::string &file, int line)
{
	log(INFO, message, file, line);
}

void Logger::warning(const std::string &message, const std::string &file, int line)
{
	log(WARNING, message, file, line);
}

void Logger::error(const std::string &message, const std::string &file, int line)
{
	log(ERROR, message, file, line);
}

void Logger::critical(const std::string &message, const std::string &file, int line)
{
	log(CRITICAL, message, file, line);
}

void Logger::logRequest(const std::string &method, const std::string &uri, const std::string &clientIP, int statusCode)
{
	std::stringstream ss;
	ss << clientIP << " - \"" << method << " " << uri << "\" " << statusCode;
	log(INFO, ss.str());
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
	log(level, ss.str(), file, line);
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