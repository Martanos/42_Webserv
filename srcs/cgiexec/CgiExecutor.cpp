#include "../../includes/CGI/CgiExecutor.hpp"
#include "../../includes/Global/PerformanceMonitor.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sys/select.h>
#include <sys/stat.h>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

CgiExecutor::CgiExecutor() : _childPid(-1), _timeoutSeconds(DEFAULT_TIMEOUT_SECONDS), _processRunning(false)
{
	// Initialize pipe file descriptors to invalid state
	for (int i = 0; i < 2; ++i)
	{
		_stdinPipe[i] = FileDescriptor();
		_stdoutPipe[i] = FileDescriptor();
		_stderrPipe[i] = FileDescriptor();
	}
}

CgiExecutor::CgiExecutor(int timeoutSeconds) : _childPid(-1), _timeoutSeconds(timeoutSeconds), _processRunning(false)
{
	// Initialize pipe file descriptors to invalid state
	for (int i = 0; i < 2; ++i)
	{
		_stdinPipe[i] = FileDescriptor();
		_stdoutPipe[i] = FileDescriptor();
		_stderrPipe[i] = FileDescriptor();
	}
}

CgiExecutor::CgiExecutor(const CgiExecutor &other)
	: _childPid(-1), _timeoutSeconds(other._timeoutSeconds), _processRunning(false)
{
	// Initialize pipe file descriptors to invalid state
	for (int i = 0; i < 2; ++i)
	{
		_stdinPipe[i] = FileDescriptor();
		_stdoutPipe[i] = FileDescriptor();
		_stderrPipe[i] = FileDescriptor();
	}
	// Note: We don't copy the process state or pipes as they are not copyable
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

CgiExecutor::~CgiExecutor()
{
	if (_processRunning)
	{
		killProcess();
	}
	closePipes();
}

/*
** --------------------------------- OPERATORS --------------------------------
*/

CgiExecutor &CgiExecutor::operator=(const CgiExecutor &other)
{
	if (this != &other)
	{
		// Clean up current state
		if (_processRunning)
		{
			killProcess();
		}
		closePipes();

		// Copy assignable members
		_timeoutSeconds = other._timeoutSeconds;

		// Reset non-copyable state
		_childPid = -1;
		_processRunning = false;

		// Initialize pipe file descriptors to invalid state
		for (int i = 0; i < 2; ++i)
		{
			_stdinPipe[i] = FileDescriptor();
			_stdoutPipe[i] = FileDescriptor();
			_stderrPipe[i] = FileDescriptor();
		}
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

CgiExecutor::ExecutionResult CgiExecutor::execute(const std::string &scriptPath, const std::string &interpreter,
												  char **envp, const std::string &inputData, std::string &outputData,
												  std::string &errorData)
{
	PERF_SCOPED_TIMER(cgi_execution);

	// Clear output buffers
	outputData.clear();
	errorData.clear();

	// Check if script exists and is executable
	if (!isFileExecutable(scriptPath))
	{
		Logger::log(Logger::ERROR, "CGI script not found or not executable: " + scriptPath);
		return ERROR_SCRIPT_NOT_EXECUTABLE;
	}

	// Setup pipes for communication
	Logger::debug("CgiExecutor: Setting up pipes for communication");
	ExecutionResult result = setupPipes();
	if (result != SUCCESS)
	{
		Logger::error("CgiExecutor: Failed to setup pipes");
		return result;
	}

	// Fork and execute the CGI script
	Logger::debug("CgiExecutor: Forking and executing CGI script");
	result = forkAndExec(scriptPath, interpreter, envp);
	if (result != SUCCESS)
	{
		Logger::error("CgiExecutor: Failed to fork and execute CGI script", __FILE__, __LINE__, "CGIExecutor::execute");
		closePipes();
		return result;
	}

	// Communicate with the child process
	Logger::debug("CgiExecutor: Communicating with child process");
	result = communicateWithChild(inputData, outputData, errorData);

	// Wait for child to complete
	Logger::debug("CgiExecutor: Waiting for child process to complete");
	ExecutionResult waitResult = waitForChild();
	if (result == SUCCESS && waitResult != SUCCESS)
	{
		result = waitResult;
	}

	closePipes();
	Logger::info("CgiExecutor: CGI execution completed with result: " + StrUtils::toString(result));
	return result;
}

void CgiExecutor::setTimeout(int seconds)
{
	_timeoutSeconds = seconds;
}

int CgiExecutor::getTimeout() const
{
	return _timeoutSeconds;
}

bool CgiExecutor::isProcessRunning() const
{
	return _processRunning;
}

void CgiExecutor::killProcess()
{
	if (_processRunning && _childPid > 0)
	{
		Logger::log(Logger::WARNING, "Killing CGI process: " + StrUtils::toString(_childPid));
		kill(_childPid, SIGTERM);

		// Force kill if still running
		if (kill(_childPid, 0) == 0) // Process still exists
		{
			kill(_childPid, SIGKILL);
		}

		// Clean up zombie process
		int status;
		waitpid(_childPid, &status, WNOHANG);

		_processRunning = false;
		_childPid = -1;
	}
}

/*
** --------------------------------- PRIVATE ----------------------------------
*/

CgiExecutor::ExecutionResult CgiExecutor::setupPipes()
{
	// Create stdin pipe using FileDescriptor wrapper
	if (!FileDescriptor::createPipe(_stdinPipe[0], _stdinPipe[1]))
	{
		Logger::log(Logger::ERROR, "Failed to create stdin pipe");
		return ERROR_PIPE_FAILED;
	}

	// Create stdout pipe using FileDescriptor wrapper
	if (!FileDescriptor::createPipe(_stdoutPipe[0], _stdoutPipe[1]))
	{
		Logger::log(Logger::ERROR, "Failed to create stdout pipe");
		_stdinPipe[0].closeDescriptor();
		_stdinPipe[1].closeDescriptor();
		return ERROR_PIPE_FAILED;
	}

	// Create stderr pipe using FileDescriptor wrapper
	if (!FileDescriptor::createPipe(_stderrPipe[0], _stderrPipe[1]))
	{
		Logger::log(Logger::ERROR, "Failed to create stderr pipe");
		_stdinPipe[0].closeDescriptor();
		_stdinPipe[1].closeDescriptor();
		_stdoutPipe[0].closeDescriptor();
		_stdoutPipe[1].closeDescriptor();
		return ERROR_PIPE_FAILED;
	}

	return SUCCESS;
}

void CgiExecutor::closePipes()
{
	for (int i = 0; i < 2; ++i)
	{
		_stdinPipe[i].closeDescriptor();
		_stdoutPipe[i].closeDescriptor();
		_stderrPipe[i].closeDescriptor();
	}
}

CgiExecutor::ExecutionResult CgiExecutor::forkAndExec(const std::string &scriptPath, const std::string &interpreter,
													  char **envp)
{
	_childPid = fork();

	if (_childPid == -1)
	{
		Logger::log(Logger::ERROR, "Fork failed: " + std::string(strerror(errno)));
		return ERROR_FORK_FAILED;
	}

	if (_childPid == 0)
	{
		// Child process

		// Redirect stdin, stdout, stderr to pipes
		if (dup2(_stdinPipe[0].getFd(), STDIN_FILENO) == -1 || dup2(_stdoutPipe[1].getFd(), STDOUT_FILENO) == -1 ||
			dup2(_stderrPipe[1].getFd(), STDERR_FILENO) == -1)
		{
			_exit(1);
		}

		closePipes();

		// Prepare arguments for execve
		std::vector<char *> args = prepareExecArgs(scriptPath, interpreter);

		// Execute the script
		if (!interpreter.empty())
		{
			execve(interpreter.c_str(), &args[0], envp);
		}
		else if (!args.empty() && args[0] != const_cast<char *>(scriptPath.c_str()))
		{
			// Has shebang interpreter
			execve(args[0], &args[0], envp);
		}
		else
		{
			// Direct execution
			execve(scriptPath.c_str(), &args[0], envp);
		}

		// If we reach here, execve failed
		_exit(1);
	}

	// Parent process
	_processRunning = true;

	// Close child ends of pipes in parent
	_stdinPipe[0].closeDescriptor();
	_stdoutPipe[1].closeDescriptor();
	_stderrPipe[1].closeDescriptor();

	return SUCCESS;
}

CgiExecutor::ExecutionResult CgiExecutor::communicateWithChild(const std::string &inputData, std::string &outputData,
															   std::string &errorData)
{
	// Write input data to child's stdin using FileDescriptor wrapper
	if (!inputData.empty())
	{
		ssize_t written = _stdinPipe[1].writePipe(inputData);
		if (written != static_cast<ssize_t>(inputData.length()))
		{
			Logger::log(Logger::ERROR, "Failed to write all input data to CGI process");
			return ERROR_WRITE_FAILED;
		}
	}

	// Close stdin pipe to signal end of input
	_stdinPipe[1].closeDescriptor();

	// Read output from child's stdout and stderr using FileDescriptor wrapper
	bool stdoutClosed = false;
	bool stderrClosed = false;

	while (!stdoutClosed || !stderrClosed)
	{
		bool dataAvailable = false;

		// Check if stdout has data ready
		if (!stdoutClosed && _stdoutPipe[0].waitForPipeReady(true, _timeoutSeconds * 1000))
		{
			std::string buffer;
			ssize_t bytesRead = _stdoutPipe[0].readPipe(buffer, MAX_OUTPUT_SIZE - outputData.length());
			if (bytesRead > 0)
			{
				outputData += buffer;
				dataAvailable = true;
				if (outputData.length() >= MAX_OUTPUT_SIZE)
				{
					Logger::log(Logger::ERROR, "CGI output too large");
					killProcess();
					return ERROR_READ_FAILED;
				}
			}
			else if (bytesRead == 0)
				stdoutClosed = true;
			else
			{
				Logger::log(Logger::ERROR, "Failed to read from stdout pipe");
				stdoutClosed = true;
			}
		}

		// Check if stderr has data ready
		if (!stderrClosed && _stderrPipe[0].waitForPipeReady(true, _timeoutSeconds * 1000))
		{
			std::string buffer;
			ssize_t bytesRead = _stderrPipe[0].readPipe(buffer, MAX_OUTPUT_SIZE - errorData.length());
			if (bytesRead > 0)
			{
				errorData += buffer;
				dataAvailable = true;
				if (errorData.length() >= MAX_OUTPUT_SIZE)
				{
					Logger::log(Logger::ERROR, "CGI error output too large");
					killProcess();
					return ERROR_READ_FAILED;
				}
			}
			else if (bytesRead == 0)
				stderrClosed = true;
			else
			{
				Logger::log(Logger::ERROR, "Failed to read from stderr pipe");
				stderrClosed = true;
			}
		}

		// If no data was available and pipes are still open, check for timeout
		if (!dataAvailable && (!stdoutClosed || !stderrClosed))
		{
			// Use a shorter timeout for individual checks to avoid hanging
			if (!_stdoutPipe[0].waitForPipeReady(true, 100) && !_stderrPipe[0].waitForPipeReady(true, 100))
			{
				// Check if child process is still running
				if (!_processRunning)
				{
					stdoutClosed = true;
					stderrClosed = true;
				}
				else
				{
					Logger::log(Logger::ERROR, "CGI process timeout");
					killProcess();
					return ERROR_TIMEOUT;
				}
			}
		}
	}

	return SUCCESS;
}

CgiExecutor::ExecutionResult CgiExecutor::waitForChild()
{
	if (!_processRunning || _childPid <= 0)
	{
		return SUCCESS;
	}

	int status;
	pid_t result = waitpid(_childPid, &status, 0);

	_processRunning = false;

	if (result == -1)
	{
		Logger::log(Logger::ERROR, "waitpid failed: " + std::string(strerror(errno)));
		return ERROR_PROCESS_CRASHED;
	}

	if (WIFEXITED(status))
	{
		int exitCode = WEXITSTATUS(status);
		if (exitCode != 0)
		{
			Logger::log(Logger::WARNING, "CGI process exited with code: " + StrUtils::toString(exitCode));
		}
		return SUCCESS;
	}
	else if (WIFSIGNALED(status))
	{
		int signal = WTERMSIG(status);
		Logger::log(Logger::ERROR, "CGI process killed by signal: " + StrUtils::toString(signal));
		return ERROR_PROCESS_CRASHED;
	}

	return SUCCESS;
}

bool CgiExecutor::isFileExecutable(const std::string &path) const
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
	{
		return false;
	}
	// Check if it's a regular file and executable
	return S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR);
}

std::string CgiExecutor::getInterpreterFromShebang(const std::string &scriptPath) const
{
	std::ifstream file(scriptPath.c_str());
	if (!file.is_open())
	{
		return "";
	}

	std::string firstLine;
	if (!std::getline(file, firstLine))
	{
		return ""; // Empty file or read error
	}

	// Early return if not a shebang
	if (firstLine.size() < 2 || firstLine.substr(0, 2) != "#!")
	{
		return "";
	}

	// Extract interpreter path (skip "#!")
	std::string interpreter = firstLine.substr(2);

	// Trim leading whitespace
	std::string::size_type start = 0;
	while (start < interpreter.size() && std::isspace(interpreter[start]))
	{
		++start;
	}

	if (start >= interpreter.size())
	{
		return ""; // Only whitespace after #!
	}

	// Find end of interpreter path (first whitespace or end)
	std::string::size_type end = start;
	while (end < interpreter.size() && !std::isspace(interpreter[end]))
	{
		++end;
	}

	return interpreter.substr(start, end - start);
}

std::vector<char *> CgiExecutor::prepareExecArgs(const std::string &scriptPath, const std::string &interpreter) const
{
	std::vector<char *> args;

	if (!interpreter.empty())
	{
		// Use interpreter
		args.push_back(const_cast<char *>(interpreter.c_str()));
		args.push_back(const_cast<char *>(scriptPath.c_str()));
	}
	else
	{
		// Try to get interpreter from shebang
		std::string shebangInterpreter = getInterpreterFromShebang(scriptPath);
		if (!shebangInterpreter.empty())
		{
			args.push_back(const_cast<char *>(shebangInterpreter.c_str()));
			args.push_back(const_cast<char *>(scriptPath.c_str()));
		}
		else
		{
			// Execute script directly
			args.push_back(const_cast<char *>(scriptPath.c_str()));
		}
	}

	args.push_back(NULL);
	return args;
}

/* ************************************************************************** */
