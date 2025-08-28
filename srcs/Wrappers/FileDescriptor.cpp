#include "FileDescriptor.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

FileDescriptor::FileDescriptor(int fd = -1) : _fd(fd), _isOpen(true)
{
	if (fd == -1)
	{
		_isOpen = false;
	}
}

FileDescriptor::FileDescriptor(const FileDescriptor &src) : _fd(src._fd), _isOpen(src._isOpen)
{
	if (src._isOpen && src._fd != -1)
	{
		_fd = dup(src._fd);
		if (_fd == -1)
		{
			std::stringstream ss;
			ss << "FileDescriptor: Failed to duplicate file descriptor: " << strerror(errno);
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}
		_isOpen = true;
	}
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

FileDescriptor::~FileDescriptor()
{
	if (_isOpen && _fd != -1)
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
		if (other._isOpen && other._fd != -1)
		{
			_fd = dup(other.getFd());
			if (_fd == -1)
			{
				std::stringstream ss;
				ss << "FileDescriptor: Failed to duplicate file descriptor: " << strerror(errno);
				Logger::log(Logger::ERROR, ss.str());
				throw std::runtime_error(ss.str());
			}
			_isOpen = true;
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
	if (_isOpen && _fd != -1)
	{
		if (close(_fd) == -1)
		{
			if (errno != EBADF)
			{
				std::stringstream ss;
				ss << "FileDescriptor: Failed to close file descriptor: " << strerror(errno);
				Logger::log(Logger::ERROR, ss.str());
				throw std::runtime_error(ss.str());
			}
			_fd = -1;
			_isOpen = false;
		}
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
	int flags = fcntl(_fd, F_GETFL, 0);
	if (flags == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to get file descriptor flags: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	flags |= SO_REUSEADDR;
	if (fcntl(_fd, F_SETFL, flags) == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to set file descriptor to reuse addr: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

void FileDescriptor::unsetReuseAddr()
{
	int flags = fcntl(_fd, F_GETFL, 0);
	if (flags == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to get file descriptor flags: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	flags &= ~SO_REUSEADDR;
	if (fcntl(_fd, F_SETFL, flags) == -1)
	{
		std::stringstream ss;
		ss << "FileDescriptor: Failed to unset file descriptor to reuse addr: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

/*
** --------------------------------- VERIFICATION ---------------------------------
*/

bool FileDescriptor::isSocket() const
{
	return _fd != -1 && S_ISSOCK(_fd);
}

bool FileDescriptor::isPipe() const
{
	return _fd != -1 && S_ISFIFO(_fd);
}

bool FileDescriptor::isFile() const
{
	return _fd != -1 && S_ISREG(_fd);
}

bool FileDescriptor::isDirectory() const
{
	return _fd != -1 && S_ISDIR(_fd);
}

bool FileDescriptor::isRegularFile() const
{
	return _fd != -1 && S_ISREG(_fd);
}

bool FileDescriptor::isSymbolicLink() const
{
	return _fd != -1 && S_ISLNK(_fd);
}

bool FileDescriptor::isValid() const
{
	return _fd != -1;
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
	return _isOpen;
}

int FileDescriptor::getFd() const
{
	return _fd;
}

FileDescriptor::operator int() const
{
	return _fd;
}

/* ************************************************************************** */
