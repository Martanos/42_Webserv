#ifndef PERFORMANCEMONITOR_HPP
#define PERFORMANCEMONITOR_HPP

#include "Logger.hpp"
#include "StringUtils.hpp"
#include <ctime>
#include <map>
#include <string>
#include <sys/resource.h>
#include <sys/time.h>
#include <vector>

// Professional performance monitoring class for C++98
class PerformanceMonitor
{
public:
	// Performance metrics structure
	struct Metrics
	{
		// Timing metrics
		double totalRequestTime;
		double averageRequestTime;
		double minRequestTime;
		double maxRequestTime;
		double totalCGITime;
		double averageCGITime;
		double totalFileReadTime;
		double averageFileReadTime;
		
		// Connection metrics
		size_t totalConnections;
		size_t activeConnections;
		size_t totalRequests;
		size_t successfulRequests;
		size_t failedRequests;
		size_t bytesTransferred;
		
		// Memory metrics
		size_t peakMemoryUsage;
		size_t currentMemoryUsage;
		size_t totalMemoryAllocated;
		
		// System metrics
		size_t fileDescriptorsUsed;
		size_t maxFileDescriptors;
		double cpuUsagePercent;
		
		// Performance counters
		size_t epollEventsProcessed;
		size_t timeoutsOccurred;
		size_t errorsOccurred;
		
		Metrics() : totalRequestTime(0), averageRequestTime(0), minRequestTime(0), maxRequestTime(0),
					totalCGITime(0), averageCGITime(0), totalFileReadTime(0), averageFileReadTime(0),
					totalConnections(0), activeConnections(0), totalRequests(0), successfulRequests(0),
					failedRequests(0), bytesTransferred(0), peakMemoryUsage(0), currentMemoryUsage(0),
					totalMemoryAllocated(0), fileDescriptorsUsed(0), maxFileDescriptors(0),
					cpuUsagePercent(0), epollEventsProcessed(0), timeoutsOccurred(0), errorsOccurred(0) {}
	};

	// Timer class for measuring execution time
	class Timer
	{
	private:
		struct timeval _startTime;
		struct timeval _endTime;
		bool _isRunning;
		std::string _operationName;
		
	public:
		Timer(const std::string &operationName = "");
		~Timer();
		
		void start();
		void stop();
		double getElapsedTime() const; // Returns time in milliseconds
		bool isRunning() const { return _isRunning; }
		const std::string &getOperationName() const { return _operationName; }
	};

private:
	// Non-instantiable singleton
	PerformanceMonitor();
	PerformanceMonitor(const PerformanceMonitor &src);
	~PerformanceMonitor();
	PerformanceMonitor &operator=(const PerformanceMonitor &rhs);

	// Static instance
	static PerformanceMonitor *_instance;
	
	// Metrics storage
	Metrics _currentMetrics;
	Metrics _sessionMetrics;
	std::vector<double> _requestTimes;
	std::vector<double> _cgiTimes;
	std::vector<double> _fileReadTimes;
	
	// Timing data
	struct timeval _sessionStartTime;
	struct timeval _lastUpdateTime;
	
	// Memory tracking
	size_t _initialMemoryUsage;
	
	// Private helper methods
	void _updateSystemMetrics();
	void _updateMemoryMetrics();
	void _updateFileDescriptorMetrics();
	double _getCurrentTime() const;
	size_t _getCurrentMemoryUsage() const;
	size_t _getFileDescriptorCount() const;
	
public:
	// Singleton access
	static PerformanceMonitor &getInstance();
	static void destroyInstance();
	
	// Timer management
	static Timer *createTimer(const std::string &operationName = "");
	static void destroyTimer(Timer *timer);
	
	// Metrics recording
	void recordRequestTime(double timeMs);
	void recordCGITime(double timeMs);
	void recordFileReadTime(double timeMs);
	void recordConnection();
	void recordDisconnection();
	void recordRequest(bool success);
	void recordBytesTransferred(size_t bytes);
	void recordEpollEvent();
	void recordTimeout();
	void recordError();
	
	// Metrics access
	const Metrics &getCurrentMetrics() const { return _currentMetrics; }
	const Metrics &getSessionMetrics() const { return _sessionMetrics; }
	
	// Statistics calculation
	void calculateStatistics();
	void resetSessionMetrics();
	
	// Reporting
	void logPerformanceReport();
	void logPerformanceSummary();
	std::string generatePerformanceReport() const;
	std::string generatePerformanceSummary() const;
	
	// System monitoring
	void updateSystemMetrics();
	void logSystemStatus();
	
	// Performance thresholds
	void setPerformanceThresholds(double maxRequestTime = 1000.0, 
								  double maxCGITime = 5000.0,
								  size_t maxMemoryUsage = 100 * 1024 * 1024); // 100MB default
	bool checkPerformanceThresholds() const;
	
	// Memory management tracking
	void recordMemoryAllocation(size_t bytes);
	void recordMemoryDeallocation(size_t bytes);
	
	// File descriptor tracking
	void recordFileDescriptorOpen();
	void recordFileDescriptorClose();
};

// RAII Timer class for automatic timing
class ScopedTimer
{
private:
	PerformanceMonitor::Timer *_timer;
	
public:
	ScopedTimer(const std::string &operationName);
	~ScopedTimer();
	
	double getElapsedTime() const;
};

// Performance measurement macros for easy use
#define PERF_START_TIMER(name) PerformanceMonitor::Timer *perf_timer_##name = PerformanceMonitor::getInstance().createTimer(#name); perf_timer_##name->start()
#define PERF_STOP_TIMER(name) perf_timer_##name->stop(); PerformanceMonitor::getInstance().recordRequestTime(perf_timer_##name->getElapsedTime()); PerformanceMonitor::getInstance().destroyTimer(perf_timer_##name)

#define PERF_SCOPED_TIMER(name) ScopedTimer perf_scoped_timer_##name(#name)

#define PERF_RECORD_REQUEST_TIME(timeMs) PerformanceMonitor::getInstance().recordRequestTime(timeMs)
#define PERF_RECORD_CGI_TIME(timeMs) PerformanceMonitor::getInstance().recordCGITime(timeMs)
#define PERF_RECORD_FILE_READ_TIME(timeMs) PerformanceMonitor::getInstance().recordFileReadTime(timeMs)
#define PERF_RECORD_CONNECTION() PerformanceMonitor::getInstance().recordConnection()
#define PERF_RECORD_DISCONNECTION() PerformanceMonitor::getInstance().recordDisconnection()
#define PERF_RECORD_REQUEST(success) PerformanceMonitor::getInstance().recordRequest(success)
#define PERF_RECORD_BYTES(bytes) PerformanceMonitor::getInstance().recordBytesTransferred(bytes)
#define PERF_RECORD_EPOLL_EVENT() PerformanceMonitor::getInstance().recordEpollEvent()
#define PERF_RECORD_TIMEOUT() PerformanceMonitor::getInstance().recordTimeout()
#define PERF_RECORD_ERROR() PerformanceMonitor::getInstance().recordError()

#define PERF_UPDATE_SYSTEM_METRICS() PerformanceMonitor::getInstance().updateSystemMetrics()
#define PERF_LOG_REPORT() PerformanceMonitor::getInstance().logPerformanceReport()
#define PERF_LOG_SUMMARY() PerformanceMonitor::getInstance().logPerformanceSummary()

#endif /* PERFORMANCEMONITOR_HPP */