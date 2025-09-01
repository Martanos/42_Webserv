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
** --------------------------------- SOCKET OPERATIONS ---------------------------------
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
** --------------------------------- COMPARATOR ---------------------------------
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
** --------------------------------- VERIFICATION ---------------------------------
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

/*
** --------------------------------- FILE OPERATIONS ---------------------------------
*/

ssize_t FileDescriptor::readFile(std::string &buffer)
{
	if (buffer.empty())
		buffer.resize(4096);
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

/* ************************************************************************** */
