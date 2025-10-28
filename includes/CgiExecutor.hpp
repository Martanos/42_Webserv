#ifndef CGIEXECUTOR_HPP
#define CGIEXECUTOR_HPP

#include "FileDescriptor.hpp"
#include "Logger.hpp"
#include <errno.h>
#include <signal.h>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

class CgiExecutor
{
public:
	enum ExecutionResult
	{
		SUCCESS = 0,
		ERROR_FORK_FAILED = 1,
		ERROR_EXEC_FAILED = 2,
		ERROR_PIPE_FAILED = 3,
		ERROR_TIMEOUT = 4,
		ERROR_SCRIPT_NOT_FOUND = 5,
		ERROR_SCRIPT_NOT_EXECUTABLE = 6,
		ERROR_WRITE_FAILED = 7,
		ERROR_READ_FAILED = 8,
		ERROR_PROCESS_CRASHED = 9
	};

private:
	static const int DEFAULT_TIMEOUT_SECONDS = 30;
	static const size_t MAX_OUTPUT_SIZE = 10 * 1024 * 1024; // 10MB limit

	pid_t _childPid;
	FileDescriptor _stdinPipe[2];  // [0] = read end, [1] = write end
	FileDescriptor _stdoutPipe[2]; // [0] = read end, [1] = write end
	FileDescriptor _stderrPipe[2]; // [0] = read end, [1] = write end

	int _timeoutSeconds;
	bool _processRunning;

public:
	CgiExecutor();
	CgiExecutor(int timeoutSeconds);
	CgiExecutor(const CgiExecutor &other);
	~CgiExecutor();

	CgiExecutor &operator=(const CgiExecutor &other);

	// Main execution method
	ExecutionResult execute(const std::string &scriptPath, const std::string &interpreter, char **envp,
							const std::string &inputData, std::string &outputData, std::string &errorData);

	// Configuration
	void setTimeout(int seconds);
	int getTimeout() const;

	// Process management
	bool isProcessRunning() const;
	void killProcess();

private:
	// Internal methods
	ExecutionResult setupPipes();
	void closePipes();
	ExecutionResult forkAndExec(const std::string &scriptPath, const std::string &interpreter, char **envp);
	ExecutionResult communicateWithChild(const std::string &inputData, std::string &outputData, std::string &errorData);
	ExecutionResult waitForChild();

	// Utility methods
	bool isFileExecutable(const std::string &path) const;
	std::string getInterpreterFromShebang(const std::string &scriptPath) const;
	std::vector<char *> prepareExecArgs(const std::string &scriptPath, const std::string &interpreter) const;
};

#endif /* CGIEXECUTOR_HPP */
