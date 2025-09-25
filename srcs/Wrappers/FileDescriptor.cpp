#include "FileDescriptor.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

FileDescriptor::FileDescriptor(int fd) : _fd(fd)
{
}

FileDescriptor::FileDescriptor(const FileDescriptor &src) : _fd(src._fd)
{
	if (isOpen() && src._fd != -1)
	{
		_fd = dup(src._fd);
		if (_fd == -1)
		{
			std::stringstream ss;
			ss << "FileDescriptor: Failed to duplicate file descriptor: " << strerror(errno);
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}
	}
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

FileDescriptor::~FileDescriptor()
{
	if (isOpen() && _fd != -1)
	{
		closeDescriptor();
	}
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

FileDescriptor &FileDescriptor::operator=(const FileDescriptor &other)
{
	if (this != &other)
	{
		closeDescriptor();
		if (other.isOpen() && other._fd != -1)
		{
			_fd = dup(other.getFd());
			if (_fd == -1)
			{
				std::stringstream ss;
				ss << "FileDescriptor: Failed to duplicate file descriptor: " << strerror(errno);
				Logger::log(Logger::ERROR, ss.str());
				throw std::runtime_error(ss.str());
			}
		}
	}
	return *this;
}

std::ostream &operator<<(std::ostream &o, FileDescriptor const &i)
{
	o << "FileDescriptor: " << i.getFd() << " isOpen: " << i.isOpen();
	return o;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void FileDescriptor::closeDescriptor()
{
	if (_fd != -1)
	{
		if (close(_fd) == -1 && errno != EBADF)
		{
			std::stringstream ss;
			ss << "FileDescriptor: Failed to close file descriptor: " << strerror(errno);
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}
		_fd = -1;
	}
}

/*
** --------------------------------- SOCKET OPERATIONS
*---------------------------------
*/

void FileDescriptor::setNonBlocking()
{
	int flags = fcntl(_fd, F_GETFL, 0);
	if (flags == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to get file descriptor flags: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	flags |= O_NONBLOCK;
	if (fcntl(_fd, F_SETFL, flags) == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to set file descriptor to non-blocking: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

void FileDescriptor::setBlocking()
{
	int flags = fcntl(_fd, F_GETFL, 0);
	if (flags == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to get file descriptor flags: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	flags &= ~O_NONBLOCK;
	if (fcntl(_fd, F_SETFL, flags) == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to set file descriptor to blocking: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

void FileDescriptor::setCloseOnExec()
{
	int flags = fcntl(_fd, F_GETFD, 0);
	if (flags == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to get file descriptor flags: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	flags |= FD_CLOEXEC;
	if (fcntl(_fd, F_SETFD, flags) == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to set file descriptor to close on exec: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

void FileDescriptor::unsetCloseOnExec()
{
	int flags = fcntl(_fd, F_GETFD, 0);
	if (flags == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to get file descriptor flags: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

void FileDescriptor::setReuseAddr()
{
	int opt = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to set SO_REUSEADDR: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

void FileDescriptor::unsetReuseAddr()
{
	int opt = 0;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to unset SO_REUSEADDR: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

/*
** --------------------------------- COMPARATOR
*---------------------------------
*/

bool FileDescriptor::operator==(const FileDescriptor &rhs) const
{
	return _fd == rhs._fd;
}

bool FileDescriptor::operator!=(const FileDescriptor &rhs) const
{
	return _fd != rhs._fd;
}

bool FileDescriptor::operator<(const FileDescriptor &rhs) const
{
	return _fd < rhs._fd;
}

bool FileDescriptor::operator>(const FileDescriptor &rhs) const
{
	return _fd > rhs._fd;
}

bool FileDescriptor::operator<=(const FileDescriptor &rhs) const
{
	return _fd <= rhs._fd;
}

bool FileDescriptor::operator>=(const FileDescriptor &rhs) const
{
	return _fd >= rhs._fd;
}

/*
** --------------------------------- VERIFICATION
*---------------------------------
*/

bool FileDescriptor::isSocket() const
{
	struct stat st;
	if (_fd == -1 || fstat(_fd, &st) == -1)
		return false;
	return S_ISSOCK(st.st_mode);
}

bool FileDescriptor::isPipe() const
{
	struct stat st;
	if (_fd == -1 || fstat(_fd, &st) == -1)
		return false;
	return S_ISFIFO(st.st_mode);
}

bool FileDescriptor::isFile() const
{
	struct stat st;
	if (_fd == -1 || fstat(_fd, &st) == -1)
		return false;
	return S_ISREG(st.st_mode);
}

bool FileDescriptor::isDirectory() const
{
	struct stat st;
	if (_fd == -1 || fstat(_fd, &st) == -1)
		return false;
	return S_ISDIR(st.st_mode);
}

bool FileDescriptor::isRegularFile() const
{
	struct stat st;
	if (_fd == -1 || fstat(_fd, &st) == -1)
		return false;
	return S_ISREG(st.st_mode);
}

bool FileDescriptor::isSymbolicLink() const
{
	struct stat st;
	if (_fd == -1 || fstat(_fd, &st) == -1)
		return false;
	return S_ISLNK(st.st_mode);
}

bool FileDescriptor::isValid() const
{
	struct stat st;
	if (_fd == -1 || fstat(_fd, &st) == -1)
		return false;
	return true;
}

/*
** --------------------------------- ACCESSORS ---------------------------------
*/

void FileDescriptor::setFd(int fd)
{
	if (isOpen())
	{
		closeDescriptor();
	}
	_fd = fd;
}

bool FileDescriptor::isOpen() const
{
	if (_fd == -1)
		return false;
	return fcntl(_fd, F_GETFD) != -1 || errno != EBADF;
}

int FileDescriptor::getFd() const
{
	return _fd;
}

FileDescriptor::operator int() const
{
	return _fd;
}

size_t FileDescriptor::getFileSize() const
{
	struct stat st;
	if (_fd == -1 || fstat(_fd, &st) == -1)
		return 0;
	return st.st_size;
}

/*
** --------------------------------- FILE OPERATIONS
*---------------------------------
*/

ssize_t FileDescriptor::readFile(std::string &buffer)
{
	if (buffer.empty())
		buffer.resize(sysconf(_SC_PAGESIZE));
	else if (!isRegularFile())
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to read file: Not a regular file";
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	ssize_t bytesRead = read(_fd, &buffer[0], buffer.size());
	if (bytesRead == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to read file: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	buffer.resize(bytesRead);
	return bytesRead;
}

ssize_t FileDescriptor::writeFile(const std::string &buffer)
{
	if (buffer.empty())
		return 0;
	ssize_t bytesWritten = write(_fd, buffer.data(), buffer.size());
	if (bytesWritten == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to write file: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	return bytesWritten;
}

/*
** --------------------------------- SOCKET OPERATIONS
*---------------------------------
*/

ssize_t FileDescriptor::receiveData(void *buffer, size_t size)
{
	if (!isSocket())
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to receive data: Not a socket";
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	return recv(_fd, buffer, size, MSG_NOSIGNAL);
}

ssize_t FileDescriptor::sendData(const std::string &buffer)
{
	if (buffer.empty())
		return 0;
	else if (!isSocket())
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to send data: Not a socket";
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	ssize_t bytesSent = send(_fd, &buffer[0], buffer.size(), 0);
	if (bytesSent == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to send data: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	return bytesSent;
}

/*
** --------------------------------- PIPE OPERATIONS
*---------------------------------
*/

ssize_t FileDescriptor::readPipe(std::string &buffer, size_t maxSize)
{
	if (!isPipe())
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to read pipe: Not a pipe";
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}

	size_t bufferSize = (maxSize > 0) ? maxSize : 4096;
	buffer.resize(bufferSize);

	ssize_t bytesRead = read(_fd, &buffer[0], bufferSize);
	if (bytesRead == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			buffer.clear();
			return 0; // No data available (non-blocking)
		}
		std::stringstream ss;
		ss << "FileDescriptor: Failed to read pipe: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	else if (bytesRead == 0)
	{
		buffer.clear();
		return 0; // EOF
	}

	buffer.resize(bytesRead);
	return bytesRead;
}

ssize_t FileDescriptor::writePipe(const std::string &buffer)
{
	if (buffer.empty())
		return 0;

	if (!isPipe())
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to write pipe: Not a pipe";
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}

	ssize_t totalWritten = 0;
	const char *data = buffer.c_str();
	size_t dataSize = buffer.length();

	while (totalWritten < static_cast<ssize_t>(dataSize))
	{
		ssize_t bytesWritten = write(_fd, data + totalWritten, dataSize - totalWritten);
		if (bytesWritten == -1)
		{
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break; // Non-blocking write would block
			if (errno == EPIPE)
			{
				std::stringstream ss;
				ss << "FileDescriptor: Pipe broken (EPIPE)";
				Logger::log(Logger::WARNING, ss.str());
				return totalWritten;
			}
			std::stringstream ss;
			ss << "FileDescriptor: Failed to write pipe: " << strerror(errno);
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}
		totalWritten += bytesWritten;
	}

	return totalWritten;
}

bool FileDescriptor::waitForPipeReady(bool forReading, int timeoutMs) const
{
	if (!isPipe())
	{
		return false;
	}

	fd_set fds;
	FD_ZERO(&fds);	   // clear the set
	FD_SET(_fd, &fds); // add file descriptor to the set

	struct timeval timeout;
	timeout.tv_sec = timeoutMs / 1000;
	timeout.tv_usec = (timeoutMs % 1000) * 1000;

	int result;
	if (forReading)
	{
		result = select(_fd + 1, &fds, NULL, NULL, &timeout);
	}
	else
	{
		// for writing
		result = select(_fd + 1, NULL, &fds, NULL, &timeout);
	}
	return result > 0 && FD_ISSET(_fd, &fds);
}

bool FileDescriptor::createPipe(FileDescriptor &readEnd, FileDescriptor &writeEnd)
{
	int pipefd[2];
	if (pipe(pipefd) == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to create pipe: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		return false;
	}

	readEnd.setFd(pipefd[0]);
	writeEnd.setFd(pipefd[1]);
	return true;
}

bool FileDescriptor::createPipeNonBlocking(FileDescriptor &readEnd, FileDescriptor &writeEnd)
{
	if (!createPipe(readEnd, writeEnd))
	{
		return false;
	}

	readEnd.setNonBlocking();
	writeEnd.setNonBlocking();
	return true;
}

/* ************************************************************************** */
