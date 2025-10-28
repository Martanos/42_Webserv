#include "../../includes/Wrapper/FileDescriptor.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

FileDescriptor::FileDescriptor() : _ctrl(new Control(-1))
{
}

FileDescriptor::FileDescriptor(int fd) : _ctrl(new Control(fd))
{
}

FileDescriptor::FileDescriptor(const FileDescriptor &src)
{
	_ctrl = src._ctrl;
	if (_ctrl)
		_ctrl->count++;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

FileDescriptor::~FileDescriptor()
{
	closeDescriptor();
}

/*
** --------------------------------- OVERLOADS ---------------------------------
*/

FileDescriptor &FileDescriptor::operator=(const FileDescriptor &other)
{
	if (this != &other)
	{
		closeDescriptor();
		_ctrl = other._ctrl;
		if (_ctrl)
			_ctrl->count++;
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
	if (_ctrl && --_ctrl->count == 0)
	{
		if (_ctrl->fd != -1)
		{
			if (::close(_ctrl->fd) == -1 && errno != EBADF)
			{
				std::stringstream ss;
				ss << __FILE__ << ":" << __LINE__
				   << ": FileDescriptor: Failed to close file descriptor: " << strerror(errno);
				Logger::error(ss.str(), __FILE__, __LINE__);
				throw std::runtime_error(ss.str());
			}
		}
		delete _ctrl;
	}
	_ctrl = NULL;
}

/*
** --------------------------------- SOCKET OPERATIONS
*---------------------------------
*/

void FileDescriptor::setNonBlocking()
{
	int flags = fcntl(_ctrl->fd, F_GETFL, 0);
	if (flags == -1)
	{
		std::stringstream ss;
		ss << __FILE__ << ":" << __LINE__
		   << ": FileDescriptor: Failed to get file descriptor flags: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	flags |= O_NONBLOCK;
	if (fcntl(_ctrl->fd, F_SETFL, flags) == -1)
	{
		std::stringstream ss;
		ss << __FILE__ << ":" << __LINE__
		   << ": FileDescriptor: Failed to set file descriptor to non-blocking: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

void FileDescriptor::setBlocking()
{
	int flags = fcntl(_ctrl->fd, F_GETFL, 0);
	if (flags == -1)
	{
		std::stringstream ss;
		ss << __FILE__ << ":" << __LINE__
		   << ": FileDescriptor: Failed to get file descriptor flags: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	flags &= ~O_NONBLOCK;
	if (fcntl(_ctrl->fd, F_SETFL, flags) == -1)
	{
		std::stringstream ss;
		ss << __FILE__ << ":" << __LINE__
		   << ": FileDescriptor: Failed to set file descriptor to blocking: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

void FileDescriptor::setCloseOnExec()
{
	int flags = fcntl(_ctrl->fd, F_GETFD, 0);
	if (flags == -1)
	{
		std::stringstream ss;
		ss << __FILE__ << ":" << __LINE__
		   << ": FileDescriptor: Failed to get file descriptor flags: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	flags |= FD_CLOEXEC;
	if (fcntl(_ctrl->fd, F_SETFD, flags) == -1)
	{
		std::stringstream ss;
		ss << __FILE__ << ":" << __LINE__
		   << ": FileDescriptor: Failed to set file descriptor to close on exec: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

void FileDescriptor::unsetCloseOnExec()
{
	int flags = fcntl(_ctrl->fd, F_GETFD, 0);
	if (flags == -1)
	{
		std::stringstream ss;
		ss << __FILE__ << ":" << __LINE__
		   << ": FileDescriptor: Failed to get file descriptor flags: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

void FileDescriptor::setReuseAddr()
{
	int opt = 1;
	if (setsockopt(_ctrl->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		std::stringstream ss;
		ss << __FILE__ << ":" << __LINE__ << ": FileDescriptor: Failed to set SO_REUSEADDR: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

void FileDescriptor::unsetReuseAddr()
{
	int opt = 0;
	if (setsockopt(_ctrl->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		std::stringstream ss;
		ss << __FILE__ << ":" << __LINE__ << ": FileDescriptor: Failed to unset SO_REUSEADDR: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

/*
** --------------------------------- COMPARATOR
*---------------------------------
*/

bool FileDescriptor::operator==(int rhs) const
{
	if (_ctrl)
	{
		return _ctrl->fd == rhs;
	}
	return false;
}

bool FileDescriptor::operator!=(int rhs) const
{
	if (_ctrl)
	{
		return _ctrl->fd != rhs;
	}
	return false;
}

bool FileDescriptor::operator<(int rhs) const
{
	if (_ctrl)
	{
		return _ctrl->fd < rhs;
	}
	return false;
}

bool FileDescriptor::operator>(int rhs) const
{
	if (_ctrl)
	{
		return _ctrl->fd > rhs;
	}
	return false;
}

bool FileDescriptor::operator<=(int rhs) const
{
	if (_ctrl)
	{
		return _ctrl->fd <= rhs;
	}
	return false;
}

bool FileDescriptor::operator>=(int rhs) const
{
	if (_ctrl)
	{
		return _ctrl->fd >= rhs;
	}
	return false;
}

bool FileDescriptor::operator==(const FileDescriptor &rhs) const
{
	if (_ctrl && rhs._ctrl)
	{
		return _ctrl->fd == rhs._ctrl->fd;
	}
	return false;
}

bool FileDescriptor::operator!=(const FileDescriptor &rhs) const
{
	if (_ctrl && rhs._ctrl)
	{
		return _ctrl->fd != rhs._ctrl->fd;
	}
	return false;
}

bool FileDescriptor::operator<(const FileDescriptor &rhs) const
{
	if (_ctrl && rhs._ctrl)
	{
		return _ctrl->fd < rhs._ctrl->fd;
	}
	return false;
}

bool FileDescriptor::operator>(const FileDescriptor &rhs) const
{
	if (_ctrl && rhs._ctrl)
	{
		return _ctrl->fd > rhs._ctrl->fd;
	}
	return false;
}

bool FileDescriptor::operator<=(const FileDescriptor &rhs) const
{
	if (_ctrl && rhs._ctrl)
	{
		return _ctrl->fd <= rhs._ctrl->fd;
	}
	return false;
}

bool FileDescriptor::operator>=(const FileDescriptor &rhs) const
{
	if (_ctrl && rhs._ctrl)
	{
		return _ctrl->fd >= rhs._ctrl->fd;
	}
	return false;
}

/*
** --------------------------------- VERIFICATION
*---------------------------------
*/

bool FileDescriptor::isSocket() const
{
	struct stat st;
	if (_ctrl == NULL || _ctrl->fd == -1 || fstat(_ctrl->fd, &st) == -1)
		return false;
	return S_ISSOCK(st.st_mode);
}

bool FileDescriptor::isPipe() const
{
	struct stat st;
	if (_ctrl == NULL || _ctrl->fd == -1 || fstat(_ctrl->fd, &st) == -1)
		return false;
	return S_ISFIFO(st.st_mode);
}

bool FileDescriptor::isFile() const
{
	struct stat st;
	if (_ctrl == NULL || _ctrl->fd == -1 || fstat(_ctrl->fd, &st) == -1)
		return false;
	return S_ISREG(st.st_mode);
}

bool FileDescriptor::isDirectory() const
{
	struct stat st;
	if (_ctrl == NULL || _ctrl->fd == -1 || fstat(_ctrl->fd, &st) == -1)
		return false;
	return S_ISDIR(st.st_mode);
}

bool FileDescriptor::isRegularFile() const
{
	struct stat st;
	if (_ctrl == NULL || _ctrl->fd == -1 || fstat(_ctrl->fd, &st) == -1)
		return false;
	return S_ISREG(st.st_mode);
}

bool FileDescriptor::isSymbolicLink() const
{
	struct stat st;
	if (_ctrl == NULL || _ctrl->fd == -1 || fstat(_ctrl->fd, &st) == -1)
		return false;
	return S_ISLNK(st.st_mode);
}

bool FileDescriptor::isValid() const
{
	struct stat st;
	if (_ctrl == NULL || _ctrl->fd == -1 || fstat(_ctrl->fd, &st) == -1)
		return false;
	return true;
}

/*
** --------------------------------- ACCESSORS ---------------------------------
*/

bool FileDescriptor::isOpen() const
{
	if (_ctrl == NULL || _ctrl->fd == -1)
		return false;
	return fcntl(_ctrl->fd, F_GETFD) != -1 || errno != EBADF;
}

int FileDescriptor::getFd() const
{
	return _ctrl ? _ctrl->fd : -1;
}

/*
** --------------------------------- FILE OPERATIONS
*---------------------------------
*/

// TODO: May overflow if the file is too large
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
	ssize_t bytesRead = read(_ctrl->fd, &buffer[0], buffer.size());
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

std::string FileDescriptor::readFile() const
{
	std::string buffer;
	buffer.resize(sysconf(_SC_PAGESIZE));
	if (!isRegularFile())
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to read file: Not a regular file";
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	ssize_t bytesRead = read(_ctrl->fd, &buffer[0], buffer.size());
	if (bytesRead == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to read file: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	buffer.resize(bytesRead);
	return buffer;
}

ssize_t FileDescriptor::writeFile(const std::string &buffer)
{
	if (buffer.empty())
		return 0;
	ssize_t bytesWritten = write(_ctrl->fd, buffer.data(), buffer.size());
	if (bytesWritten == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to write file: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	return bytesWritten;
}

ssize_t FileDescriptor::writeFile(const std::vector<char> &buffer, std::vector<char>::iterator start,
								  std::vector<char>::iterator end)
{
	(void)buffer; // Suppress unused parameter warning
	if (start == end)
		return 0;
	ssize_t bytesWritten = write(_ctrl->fd, &(*start), end - start);
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
	return recv(_ctrl->fd, buffer, size, MSG_NOSIGNAL);
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
	ssize_t bytesSent = send(_ctrl->fd, &buffer[0], buffer.size(), 0);
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

	ssize_t bytesRead = read(_ctrl->fd, &buffer[0], bufferSize);
	if (bytesRead == -1)
	{
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
		ssize_t bytesWritten = write(_ctrl->fd, data + totalWritten, dataSize - totalWritten);
		if (bytesWritten == -1)
		{
			std::stringstream ss;
			ss << "FileDescriptor: Failed to write pipe: " << strerror(errno);
			Logger::error(ss.str(), __FILE__, __LINE__);
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
	FD_ZERO(&fds);			 // clear the set
	FD_SET(_ctrl->fd, &fds); // add file descriptor to the set

	struct timeval timeout;
	timeout.tv_sec = timeoutMs / 1000;
	timeout.tv_usec = (timeoutMs % 1000) * 1000;

	int result;
	if (forReading)
	{
		result = select(_ctrl->fd + 1, &fds, NULL, NULL, &timeout);
	}
	else
	{
		// for writing
		result = select(_ctrl->fd + 1, NULL, &fds, NULL, &timeout);
	}
	return result > 0 && FD_ISSET(_ctrl->fd, &fds);
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

	readEnd = FileDescriptor(pipefd[0]);
	readEnd.setNonBlocking();
	writeEnd = FileDescriptor(pipefd[1]);
	writeEnd.setNonBlocking();
	return true;
}

/*
** --------------------------------- FACTORY METHODS
*---------------------------------
*/

FileDescriptor FileDescriptor::createSocket(int domain, int type, int protocol)
{
	int fd = socket(domain, type, protocol);
	if (fd == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to create socket: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	FileDescriptor newFd(fd);
	newFd.setNonBlocking();
	return newFd;
}

FileDescriptor FileDescriptor::createFromAccept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	int fd = accept(sockfd, addr, addrlen);
	if (fd == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to create socket: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	FileDescriptor newFd(fd);
	newFd.setNonBlocking();
	return newFd;
}

FileDescriptor FileDescriptor::createFromOpen(const char *pathname, int flags)
{
	int fd = open(pathname, flags);
	if (fd == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to create socket: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	FileDescriptor newFd(fd);
	newFd.setNonBlocking();
	return newFd;
}

FileDescriptor FileDescriptor::createFromOpen(const char *pathname, int flags, mode_t mode)
{
	int fd = open(pathname, flags, mode);
	if (fd == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to create socket: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	FileDescriptor newFd(fd);
	newFd.setNonBlocking();
	return newFd;
}

FileDescriptor FileDescriptor::createFromDup(int oldfd)
{
	int fd = dup(oldfd);
	if (fd == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to create socket: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	FileDescriptor newFd(fd);
	newFd.setNonBlocking();
	return newFd;
}

FileDescriptor FileDescriptor::createFromDup2(int oldfd, int newfd)
{
	int fd = dup2(oldfd, newfd);
	if (fd == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to dup2 file descriptor: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	FileDescriptor newFd(fd);
	newFd.setNonBlocking();
	return newFd;
}

// TODO: Directory operations
FileDescriptor FileDescriptor::createFromOpendir(const char *name)
{
	DIR *fd = opendir(name);
	if (fd == NULL)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to create socket: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	return FileDescriptor(dirfd(fd));
}

FileDescriptor FileDescriptor::createEpoll(int flags)
{
	int fd = epoll_create1(flags);
	if (fd == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to create epoll: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	return FileDescriptor(fd);
}

/* ************************************************************************** */
