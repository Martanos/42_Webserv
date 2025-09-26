#include "../../includes/PerformanceMonitor.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <sys/resource.h>
#include <unistd.h>

// Static member initialization
PerformanceMonitor *PerformanceMonitor::_instance = NULL;

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

PerformanceMonitor::PerformanceMonitor()
{
	gettimeofday(&_sessionStartTime, NULL);
	gettimeofday(&_lastUpdateTime, NULL);
	_initialMemoryUsage = _getCurrentMemoryUsage();
	
	Logger::info("PerformanceMonitor: Performance monitoring initialized", __FILE__, __LINE__);
}

PerformanceMonitor::PerformanceMonitor(const PerformanceMonitor &src)
{
	(void)src;
	throw std::runtime_error("PerformanceMonitor: Copy constructor called");
}

PerformanceMonitor::~PerformanceMonitor()
{
	Logger::info("PerformanceMonitor: Performance monitoring shutdown", __FILE__, __LINE__);
}

PerformanceMonitor &PerformanceMonitor::operator=(const PerformanceMonitor &rhs)
{
	(void)rhs;
	throw std::runtime_error("PerformanceMonitor: Assignment operator called");
}

/*
** ------------------------------- SINGLETON METHODS --------------------------------
*/

PerformanceMonitor &PerformanceMonitor::getInstance()
{
	if (_instance == NULL)
	{
		_instance = new PerformanceMonitor();
	}
	return *_instance;
}

void PerformanceMonitor::destroyInstance()
{
	if (_instance != NULL)
	{
		delete _instance;
		_instance = NULL;
	}
}

/*
** ------------------------------- TIMER METHODS --------------------------------
*/

PerformanceMonitor::Timer::Timer(const std::string &operationName)
	: _isRunning(false), _operationName(operationName)
{
	_startTime.tv_sec = 0;
	_startTime.tv_usec = 0;
	_endTime.tv_sec = 0;
	_endTime.tv_usec = 0;
}

PerformanceMonitor::Timer::~Timer()
{
	if (_isRunning)
	{
		stop();
	}
}

void PerformanceMonitor::Timer::start()
{
	gettimeofday(&_startTime, NULL);
	_isRunning = true;
}

void PerformanceMonitor::Timer::stop()
{
	if (_isRunning)
	{
		gettimeofday(&_endTime, NULL);
		_isRunning = false;
	}
}

double PerformanceMonitor::Timer::getElapsedTime() const
{
	if (!_isRunning)
	{
		return ((_endTime.tv_sec - _startTime.tv_sec) * 1000.0) + 
			   ((_endTime.tv_usec - _startTime.tv_usec) / 1000.0);
	}
	else
	{
		struct timeval currentTime;
		gettimeofday(&currentTime, NULL);
		return ((currentTime.tv_sec - _startTime.tv_sec) * 1000.0) + 
			   ((currentTime.tv_usec - _startTime.tv_usec) / 1000.0);
	}
}

PerformanceMonitor::Timer *PerformanceMonitor::createTimer(const std::string &operationName)
{
	return new Timer(operationName);
}

void PerformanceMonitor::destroyTimer(Timer *timer)
{
	if (timer != NULL)
	{
		delete timer;
	}
}

/*
** ------------------------------- SCOPED TIMER --------------------------------
*/

ScopedTimer::ScopedTimer(const std::string &operationName)
	: _timer(PerformanceMonitor::getInstance().createTimer(operationName))
{
	_timer->start();
}

ScopedTimer::~ScopedTimer()
{
	_timer->stop();
	PerformanceMonitor::getInstance().recordRequestTime(_timer->getElapsedTime());
	PerformanceMonitor::getInstance().destroyTimer(_timer);
}

double ScopedTimer::getElapsedTime() const
{
	return _timer->getElapsedTime();
}

/*
** ------------------------------- METRICS RECORDING --------------------------------
*/

void PerformanceMonitor::recordRequestTime(double timeMs)
{
	_requestTimes.push_back(timeMs);
	_currentMetrics.totalRequestTime += timeMs;
	_sessionMetrics.totalRequestTime += timeMs;
	
	if (_currentMetrics.minRequestTime == 0 || timeMs < _currentMetrics.minRequestTime)
		_currentMetrics.minRequestTime = timeMs;
	if (timeMs > _currentMetrics.maxRequestTime)
		_currentMetrics.maxRequestTime = timeMs;
	
	calculateStatistics();
}

void PerformanceMonitor::recordCGITime(double timeMs)
{
	_cgiTimes.push_back(timeMs);
	_currentMetrics.totalCGITime += timeMs;
	_sessionMetrics.totalCGITime += timeMs;
	
	calculateStatistics();
}

void PerformanceMonitor::recordFileReadTime(double timeMs)
{
	_fileReadTimes.push_back(timeMs);
	_currentMetrics.totalFileReadTime += timeMs;
	_sessionMetrics.totalFileReadTime += timeMs;
	
	calculateStatistics();
}

void PerformanceMonitor::recordConnection()
{
	_currentMetrics.totalConnections++;
	_sessionMetrics.totalConnections++;
	_currentMetrics.activeConnections++;
	_sessionMetrics.activeConnections++;
	
	Logger::debug("PerformanceMonitor: Connection recorded. Active: " + 
				  StringUtils::toString(_currentMetrics.activeConnections), __FILE__, __LINE__);
}

void PerformanceMonitor::recordDisconnection()
{
	if (_currentMetrics.activeConnections > 0)
	{
		_currentMetrics.activeConnections--;
		_sessionMetrics.activeConnections--;
	}
	
	Logger::debug("PerformanceMonitor: Disconnection recorded. Active: " + 
				  StringUtils::toString(_currentMetrics.activeConnections), __FILE__, __LINE__);
}

void PerformanceMonitor::recordRequest(bool success)
{
	_currentMetrics.totalRequests++;
	_sessionMetrics.totalRequests++;
	
	if (success)
	{
		_currentMetrics.successfulRequests++;
		_sessionMetrics.successfulRequests++;
	}
	else
	{
		_currentMetrics.failedRequests++;
		_sessionMetrics.failedRequests++;
	}
}

void PerformanceMonitor::recordBytesTransferred(size_t bytes)
{
	_currentMetrics.bytesTransferred += bytes;
	_sessionMetrics.bytesTransferred += bytes;
}

void PerformanceMonitor::recordEpollEvent()
{
	_currentMetrics.epollEventsProcessed++;
	_sessionMetrics.epollEventsProcessed++;
}

void PerformanceMonitor::recordTimeout()
{
	_currentMetrics.timeoutsOccurred++;
	_sessionMetrics.timeoutsOccurred++;
}

void PerformanceMonitor::recordError()
{
	_currentMetrics.errorsOccurred++;
	_sessionMetrics.errorsOccurred++;
}

void PerformanceMonitor::recordMemoryAllocation(size_t bytes)
{
	_currentMetrics.totalMemoryAllocated += bytes;
	_sessionMetrics.totalMemoryAllocated += bytes;
	_updateMemoryMetrics();
}

void PerformanceMonitor::recordMemoryDeallocation(size_t bytes)
{
	if (_currentMetrics.totalMemoryAllocated >= bytes)
	{
		_currentMetrics.totalMemoryAllocated -= bytes;
		_sessionMetrics.totalMemoryAllocated -= bytes;
	}
	_updateMemoryMetrics();
}

void PerformanceMonitor::recordFileDescriptorOpen()
{
	_currentMetrics.fileDescriptorsUsed++;
	_sessionMetrics.fileDescriptorsUsed++;
}

void PerformanceMonitor::recordFileDescriptorClose()
{
	if (_currentMetrics.fileDescriptorsUsed > 0)
	{
		_currentMetrics.fileDescriptorsUsed--;
		_sessionMetrics.fileDescriptorsUsed--;
	}
}

/*
** ------------------------------- STATISTICS CALCULATION --------------------------------
*/

void PerformanceMonitor::calculateStatistics()
{
	// Calculate request time statistics
	if (!_requestTimes.empty())
	{
		_currentMetrics.averageRequestTime = _currentMetrics.totalRequestTime / _requestTimes.size();
		_sessionMetrics.averageRequestTime = _sessionMetrics.totalRequestTime / _requestTimes.size();
	}
	
	// Calculate CGI time statistics
	if (!_cgiTimes.empty())
	{
		_currentMetrics.averageCGITime = _currentMetrics.totalCGITime / _cgiTimes.size();
		_sessionMetrics.averageCGITime = _sessionMetrics.totalCGITime / _cgiTimes.size();
	}
	
	// Calculate file read time statistics
	if (!_fileReadTimes.empty())
	{
		_currentMetrics.averageFileReadTime = _currentMetrics.totalFileReadTime / _fileReadTimes.size();
		_sessionMetrics.averageFileReadTime = _sessionMetrics.totalFileReadTime / _fileReadTimes.size();
	}
}

void PerformanceMonitor::resetSessionMetrics()
{
	_sessionMetrics = Metrics();
	_requestTimes.clear();
	_cgiTimes.clear();
	_fileReadTimes.clear();
	gettimeofday(&_sessionStartTime, NULL);
	
	Logger::info("PerformanceMonitor: Session metrics reset", __FILE__, __LINE__);
}

/*
** ------------------------------- SYSTEM MONITORING --------------------------------
*/

void PerformanceMonitor::updateSystemMetrics()
{
	_updateMemoryMetrics();
	_updateFileDescriptorMetrics();
	_updateSystemMetrics();
	gettimeofday(&_lastUpdateTime, NULL);
}

void PerformanceMonitor::_updateMemoryMetrics()
{
	size_t currentMemory = _getCurrentMemoryUsage();
	_currentMetrics.currentMemoryUsage = currentMemory;
	_sessionMetrics.currentMemoryUsage = currentMemory;
	
	if (currentMemory > _currentMetrics.peakMemoryUsage)
	{
		_currentMetrics.peakMemoryUsage = currentMemory;
		_sessionMetrics.peakMemoryUsage = currentMemory;
	}
}

void PerformanceMonitor::_updateFileDescriptorMetrics()
{
	size_t fdCount = _getFileDescriptorCount();
	_currentMetrics.fileDescriptorsUsed = fdCount;
	_sessionMetrics.fileDescriptorsUsed = fdCount;
	
	// Get max file descriptors from system limits
	struct rlimit rlim;
	if (getrlimit(RLIMIT_NOFILE, &rlim) == 0)
	{
		_currentMetrics.maxFileDescriptors = rlim.rlim_cur;
		_sessionMetrics.maxFileDescriptors = rlim.rlim_cur;
	}
}

void PerformanceMonitor::_updateSystemMetrics()
{
	// CPU usage calculation would require more complex implementation
	// For now, we'll set it to 0 and can be enhanced later
	_currentMetrics.cpuUsagePercent = 0.0;
	_sessionMetrics.cpuUsagePercent = 0.0;
}

size_t PerformanceMonitor::_getCurrentMemoryUsage() const
{
	struct rusage usage;
	if (getrusage(RUSAGE_SELF, &usage) == 0)
	{
		return usage.ru_maxrss * 1024; // Convert from KB to bytes
	}
	return 0;
}

size_t PerformanceMonitor::_getFileDescriptorCount() const
{
	// Count open file descriptors by checking /proc/self/fd
	// This is a simplified implementation
	return _currentMetrics.fileDescriptorsUsed;
}

double PerformanceMonitor::_getCurrentTime() const
{
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return ((currentTime.tv_sec - _sessionStartTime.tv_sec) * 1000.0) + 
		   ((currentTime.tv_usec - _sessionStartTime.tv_usec) / 1000.0);
}

/*
** ------------------------------- REPORTING --------------------------------
*/

void PerformanceMonitor::logPerformanceReport()
{
	Logger::info("=== PERFORMANCE REPORT ===", __FILE__, __LINE__);
	Logger::info(generatePerformanceReport(), __FILE__, __LINE__);
	Logger::info("=== END PERFORMANCE REPORT ===", __FILE__, __LINE__);
}

void PerformanceMonitor::logPerformanceSummary()
{
	Logger::info("=== PERFORMANCE SUMMARY ===", __FILE__, __LINE__);
	Logger::info(generatePerformanceSummary(), __FILE__, __LINE__);
	Logger::info("=== END PERFORMANCE SUMMARY ===", __FILE__, __LINE__);
}

std::string PerformanceMonitor::generatePerformanceReport() const
{
	std::stringstream ss;
	
	ss << "Session Duration: " << std::fixed << std::setprecision(2) << _getCurrentTime() << " ms\n";
	ss << "Total Connections: " << _sessionMetrics.totalConnections << "\n";
	ss << "Active Connections: " << _sessionMetrics.activeConnections << "\n";
	ss << "Total Requests: " << _sessionMetrics.totalRequests << "\n";
	ss << "Successful Requests: " << _sessionMetrics.successfulRequests << "\n";
	ss << "Failed Requests: " << _sessionMetrics.failedRequests << "\n";
	ss << "Bytes Transferred: " << _sessionMetrics.bytesTransferred << "\n";
	ss << "Epoll Events Processed: " << _sessionMetrics.epollEventsProcessed << "\n";
	ss << "Timeouts: " << _sessionMetrics.timeoutsOccurred << "\n";
	ss << "Errors: " << _sessionMetrics.errorsOccurred << "\n";
	ss << "\n";
	ss << "Request Timing:\n";
	ss << "  Total Time: " << std::fixed << std::setprecision(2) << _sessionMetrics.totalRequestTime << " ms\n";
	ss << "  Average Time: " << std::fixed << std::setprecision(2) << _sessionMetrics.averageRequestTime << " ms\n";
	ss << "  Min Time: " << std::fixed << std::setprecision(2) << _sessionMetrics.minRequestTime << " ms\n";
	ss << "  Max Time: " << std::fixed << std::setprecision(2) << _sessionMetrics.maxRequestTime << " ms\n";
	ss << "\n";
	ss << "CGI Timing:\n";
	ss << "  Total Time: " << std::fixed << std::setprecision(2) << _sessionMetrics.totalCGITime << " ms\n";
	ss << "  Average Time: " << std::fixed << std::setprecision(2) << _sessionMetrics.averageCGITime << " ms\n";
	ss << "\n";
	ss << "File Read Timing:\n";
	ss << "  Total Time: " << std::fixed << std::setprecision(2) << _sessionMetrics.totalFileReadTime << " ms\n";
	ss << "  Average Time: " << std::fixed << std::setprecision(2) << _sessionMetrics.averageFileReadTime << " ms\n";
	ss << "\n";
	ss << "Memory Usage:\n";
	ss << "  Current: " << (_sessionMetrics.currentMemoryUsage / 1024) << " KB\n";
	ss << "  Peak: " << (_sessionMetrics.peakMemoryUsage / 1024) << " KB\n";
	ss << "  Total Allocated: " << (_sessionMetrics.totalMemoryAllocated / 1024) << " KB\n";
	ss << "\n";
	ss << "File Descriptors:\n";
	ss << "  Used: " << _sessionMetrics.fileDescriptorsUsed << "\n";
	ss << "  Max: " << _sessionMetrics.maxFileDescriptors << "\n";
	
	return ss.str();
}

std::string PerformanceMonitor::generatePerformanceSummary() const
{
	std::stringstream ss;
	
	double sessionTimeMs = _getCurrentTime();
	double requestsPerSecond = 0;
	double bytesPerSecond = 0;
	
	if (sessionTimeMs > 0)
	{
		requestsPerSecond = (_sessionMetrics.totalRequests * 1000.0) / sessionTimeMs;
		bytesPerSecond = (_sessionMetrics.bytesTransferred * 1000.0) / sessionTimeMs;
	}
	
	ss << "Performance Summary:\n";
	ss << "  Requests/sec: " << std::fixed << std::setprecision(2) << requestsPerSecond << "\n";
	ss << "  Bytes/sec: " << std::fixed << std::setprecision(2) << bytesPerSecond << "\n";
	ss << "  Avg Response Time: " << std::fixed << std::setprecision(2) << _sessionMetrics.averageRequestTime << " ms\n";
	ss << "  Success Rate: " << std::fixed << std::setprecision(1) << 
		(_sessionMetrics.totalRequests > 0 ? 
		 (100.0 * _sessionMetrics.successfulRequests / _sessionMetrics.totalRequests) : 0.0) << "%\n";
	ss << "  Active Connections: " << _sessionMetrics.activeConnections << "\n";
	ss << "  Memory Usage: " << (_sessionMetrics.currentMemoryUsage / 1024) << " KB\n";
	
	return ss.str();
}

void PerformanceMonitor::logSystemStatus()
{
	updateSystemMetrics();
	
	Logger::info("System Status:", __FILE__, __LINE__);
	Logger::info("  Memory Usage: " + StringUtils::toString(_currentMetrics.currentMemoryUsage / 1024) + " KB", __FILE__, __LINE__);
	Logger::info("  Peak Memory: " + StringUtils::toString(_currentMetrics.peakMemoryUsage / 1024) + " KB", __FILE__, __LINE__);
	Logger::info("  File Descriptors: " + StringUtils::toString(_currentMetrics.fileDescriptorsUsed) + "/" + 
				 StringUtils::toString(_currentMetrics.maxFileDescriptors), __FILE__, __LINE__);
	Logger::info("  Active Connections: " + StringUtils::toString(_currentMetrics.activeConnections), __FILE__, __LINE__);
}

/*
** ------------------------------- PERFORMANCE THRESHOLDS --------------------------------
*/

void PerformanceMonitor::setPerformanceThresholds(double maxRequestTime, double maxCGITime, size_t maxMemoryUsage)
{
	// This would be implemented to set thresholds and check them
	// For now, we'll just log the thresholds
	Logger::info("PerformanceMonitor: Thresholds set - Max Request Time: " + 
				 StringUtils::toString(maxRequestTime) + " ms, Max CGI Time: " + 
				 StringUtils::toString(maxCGITime) + " ms, Max Memory: " + 
				 StringUtils::toString(maxMemoryUsage / 1024) + " KB", __FILE__, __LINE__);
}

bool PerformanceMonitor::checkPerformanceThresholds() const
{
	// Check if any metrics exceed thresholds
	// This would be implemented based on the thresholds set
	return true; // Placeholder implementation
}