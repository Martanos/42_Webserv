#include "../../includes/Cgi/CgiExecutor.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/PerformanceMonitor.hpp"
#include "../../includes/Utils/StrUtils.hpp"
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <sys/select.h>
#include <sys/stat.h>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

void CgiExecutor::_initPipes()
{
	for (int i = 0; i < 2; ++i)
	{
		_stdinPipe[i] = FileDescriptor();
		_stdoutPipe[i] = FileDescriptor();
		_stderrPipe[i] = FileDescriptor();
	}
}

CgiExecutor::CgiExecutor() : _childPid(-1), _timeoutSeconds(DEFAULT_TIMEOUT_SECONDS), _processRunning(false)
{
	_initPipes();
}

CgiExecutor::CgiExecutor(int timeoutSeconds) : _childPid(-1), _timeoutSeconds(timeoutSeconds), _processRunning(false)
{
	_initPipes();
}

CgiExecutor::CgiExecutor(const CgiExecutor &other)
	: _childPid(-1), _timeoutSeconds(other._timeoutSeconds), _processRunning(false)
{
	_initPipes();
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
	Logger::debug("CgiExecutor: Setting up pipes for communication", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	ExecutionResult result = setupPipes();
	if (result != SUCCESS)
	{
		Logger::error("CgiExecutor: Failed to setup pipes", __FILE__, __LINE__, __PRETTY_FUNCTION__);
		return result;
	}

	// Fork and execute the CGI script
	Logger::debug("CgiExecutor: Forking and executing CGI script", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	result = forkAndExec(scriptPath, interpreter, envp);
	if (result != SUCCESS)
	{
		Logger::error("CgiExecutor: Failed to fork and execute CGI script", __FILE__, __LINE__, __FUNCTION__);
		closePipes();
		return result;
	}

	// Communicate with the child process
	Logger::debug("CgiExecutor: Communicating with child process", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	result = communicateWithChild(inputData, outputData, errorData);

	// Wait for child to complete
	Logger::debug("CgiExecutor: Waiting for child process to complete", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	ExecutionResult waitResult = waitForChild();
	if (result == SUCCESS && waitResult != SUCCESS)
	{
		result = waitResult;
	}

	closePipes();
	Logger::info("CgiExecutor: CGI execution completed with result: " + StrUtils::toString(result), __FILE__, __LINE__,
				 __PRETTY_FUNCTION__);
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

		// Prepare arguments - must be done in child to keep strings alive
		std::string shebangInterpreter;
		if (interpreter.empty())
		{
			shebangInterpreter = getInterpreterFromShebang(scriptPath);
			// Fallback for PHP files
			if (shebangInterpreter.empty() && scriptPath.size() > 4 &&
				scriptPath.substr(scriptPath.size() - 4) == ".php")
			{
				shebangInterpreter = "/usr/bin/php";
			}
		}

		char *args[3];
		if (!interpreter.empty())
		{
			// Use provided interpreter
			args[0] = const_cast<char *>(interpreter.c_str());
			args[1] = const_cast<char *>(scriptPath.c_str());
			args[2] = NULL;
			execve(interpreter.c_str(), args, envp);
		}
		else if (!shebangInterpreter.empty())
		{
			// Use shebang interpreter
			args[0] = const_cast<char *>(shebangInterpreter.c_str());
			args[1] = const_cast<char *>(scriptPath.c_str());
			args[2] = NULL;
			execve(shebangInterpreter.c_str(), args, envp);
		}
		else
		{
			// Direct execution
			args[0] = const_cast<char *>(scriptPath.c_str());
			args[1] = NULL;
			execve(scriptPath.c_str(), args, envp);
		}

		// If we reach here, execve failed - write to stderr before exit
		const char *errMsg = "CGI execve failed: ";
		write(STDERR_FILENO, errMsg, strlen(errMsg));
		write(STDERR_FILENO, strerror(errno), strlen(strerror(errno)));
		write(STDERR_FILENO, "\n", 1);
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
	// Use select() to handle reading and writing simultaneously
	// This prevents deadlock when writing large amounts of data

	size_t inputOffset = 0;
	bool stdinClosed = inputData.empty();
	bool stdoutClosed = false;
	bool stderrClosed = false;

	// Set stdin pipe to non-blocking mode
	if (!stdinClosed)
	{
		int flags = fcntl(_stdinPipe[1].getFd(), F_GETFL, 0);
		fcntl(_stdinPipe[1].getFd(), F_SETFL, flags | O_NONBLOCK);
	}

	// Set stdout and stderr pipes to non-blocking mode
	int stdoutFlags = fcntl(_stdoutPipe[0].getFd(), F_GETFL, 0);
	fcntl(_stdoutPipe[0].getFd(), F_SETFL, stdoutFlags | O_NONBLOCK);
	int stderrFlags = fcntl(_stderrPipe[0].getFd(), F_GETFL, 0);
	fcntl(_stderrPipe[0].getFd(), F_SETFL, stderrFlags | O_NONBLOCK);

	time_t startTime = time(NULL);

	while (!stdoutClosed || !stderrClosed || !stdinClosed)
	{
		// Check for timeout
		if (time(NULL) - startTime > _timeoutSeconds)
		{
			Logger::log(Logger::ERROR, "CGI process timeout during communication");
			killProcess();
			return ERROR_TIMEOUT;
		}

		fd_set readFds, writeFds;
		FD_ZERO(&readFds);
		FD_ZERO(&writeFds);

		int maxFd = -1;

		// Add stdin to write set if we still have data to write
		if (!stdinClosed)
		{
			FD_SET(_stdinPipe[1].getFd(), &writeFds);
			if (_stdinPipe[1].getFd() > maxFd)
				maxFd = _stdinPipe[1].getFd();
		}

		// Add stdout to read set
		if (!stdoutClosed)
		{
			FD_SET(_stdoutPipe[0].getFd(), &readFds);
			if (_stdoutPipe[0].getFd() > maxFd)
				maxFd = _stdoutPipe[0].getFd();
		}

		// Add stderr to read set
		if (!stderrClosed)
		{
			FD_SET(_stderrPipe[0].getFd(), &readFds);
			if (_stderrPipe[0].getFd() > maxFd)
				maxFd = _stderrPipe[0].getFd();
		}

		if (maxFd < 0)
			break;

		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		int ready = select(maxFd + 1, &readFds, &writeFds, NULL, &timeout);
		if (ready < 0)
		{
			if (errno == EINTR)
				continue;
			Logger::log(Logger::ERROR, "select() failed in CGI communication");
			return ERROR_READ_FAILED;
		}

		// Check if child process has exited (with WNOHANG to not block)
		// If child has exited and we have no more data to read, we can stop
		if (ready == 0 && _childPid > 0)
		{
			int status;
			pid_t result = waitpid(_childPid, &status, WNOHANG);
			if (result > 0)
			{
				// Child has exited, do one final read attempt then close pipes
				Logger::debug("CgiExecutor: Child process has exited, draining remaining data", __FILE__, __LINE__,
							  __PRETTY_FUNCTION__);
				_processRunning = false;
				// Read any remaining data from stdout/stderr
				if (!stdoutClosed)
				{
					char buffer[65536];
					while (true)
					{
						ssize_t bytesRead = read(_stdoutPipe[0].getFd(), buffer, sizeof(buffer));
						if (bytesRead > 0)
							outputData.append(buffer, bytesRead);
						else
							break;
					}
					stdoutClosed = true;
				}
				if (!stderrClosed)
				{
					char buffer[65536];
					while (true)
					{
						ssize_t bytesRead = read(_stderrPipe[0].getFd(), buffer, sizeof(buffer));
						if (bytesRead > 0)
							errorData.append(buffer, bytesRead);
						else
							break;
					}
					stderrClosed = true;
				}
				break;
			}
		}

		// Handle stdin writing
		if (!stdinClosed && FD_ISSET(_stdinPipe[1].getFd(), &writeFds))
		{
			size_t remaining = inputData.length() - inputOffset;
			size_t toWrite = (remaining > 65536) ? 65536 : remaining; // Write in 64KB chunks

			ssize_t written = write(_stdinPipe[1].getFd(), inputData.c_str() + inputOffset, toWrite);
			if (written > 0)
			{
				inputOffset += written;
				if (inputOffset >= inputData.length())
				{
					_stdinPipe[1].closeDescriptor();
					stdinClosed = true;
				}
			}
			else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
			{
				Logger::log(Logger::ERROR, "Failed to write to CGI stdin: " + std::string(strerror(errno)) +
											   " (wrote " + StrUtils::toString(inputOffset) + "/" +
											   StrUtils::toString(inputData.length()) + " bytes)");
				_stdinPipe[1].closeDescriptor();
				stdinClosed = true;
			}
		}

		// Handle stdout reading
		if (!stdoutClosed && FD_ISSET(_stdoutPipe[0].getFd(), &readFds))
		{
			char buffer[65536];
			ssize_t bytesRead = read(_stdoutPipe[0].getFd(), buffer, sizeof(buffer));
			if (bytesRead > 0)
			{
				outputData.append(buffer, bytesRead);
				if (outputData.length() >= MAX_OUTPUT_SIZE)
				{
					Logger::log(Logger::ERROR, "CGI output too large");
					killProcess();
					return ERROR_READ_FAILED;
				}
			}
			else if (bytesRead == 0)
			{
				stdoutClosed = true;
			}
			else if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				stdoutClosed = true;
			}
		}

		// Handle stderr reading
		if (!stderrClosed && FD_ISSET(_stderrPipe[0].getFd(), &readFds))
		{
			char buffer[65536];
			ssize_t bytesRead = read(_stderrPipe[0].getFd(), buffer, sizeof(buffer));
			if (bytesRead > 0)
			{
				errorData.append(buffer, bytesRead);
				if (errorData.length() >= MAX_OUTPUT_SIZE)
				{
					Logger::log(Logger::ERROR, "CGI error output too large");
					killProcess();
					return ERROR_READ_FAILED;
				}
			}
			else if (bytesRead == 0)
			{
				stderrClosed = true;
			}
			else if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				stderrClosed = true;
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

/* ************************************************************************** */
