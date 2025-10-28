#include "../../includes/Core/EpollManager.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include <cerrno>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <sys/epoll.h>
#include <vector>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

EpollManager::EpollManager(const EpollManager &src) : _epollFd(src._epollFd)
{
	(void)src;
	throw std::runtime_error("EpollManager: Copy constructor called");
}

EpollManager &EpollManager::operator=(const EpollManager &rhs)
{
	if (this != &rhs)
	{
		_epollFd = rhs._epollFd;
	}
	return *this;
}

EpollManager::EpollManager()
{
	_epollFd = FileDescriptor::createEpoll(EPOLL_CLOEXEC);
	if (!_epollFd.isValid())
	{
		std::stringstream ss;
		ss << "Failed to create epoll instance";
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	Logger::info("EpollManager: Epoll instance created successfully with fd " + StrUtils::toString(_epollFd.getFd()));
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

EpollManager::~EpollManager()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

/*
** --------------------------------- METHODS ----------------------------------
*/

void EpollManager::addFd(FileDescriptor fd, uint32_t events)
{
	epoll_event event;
	event.events = events;
	event.data.fd = fd.getFd();

	Logger::debug("EpollManager: Adding fd " + StrUtils::toString(fd.getFd()) + " with events " + StrUtils::toString(events));
	
	if (epoll_ctl(_epollFd.getFd(), EPOLL_CTL_ADD, fd.getFd(), &event) == -1)
	{
		std::stringstream ss;
		ss << "epoll_ctl ADD failed for fd: " << fd.getFd();
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	
	Logger::debug("EpollManager: Successfully added fd " + StrUtils::toString(fd.getFd()) + " to epoll");
}

void EpollManager::modifyFd(FileDescriptor fd, uint32_t events)
{
	epoll_event event;
	event.events = events;
	event.data.fd = fd.getFd();

	Logger::debug("EpollManager: Modifying fd " + StrUtils::toString(fd.getFd()) + " with events " + StrUtils::toString(events));
	
	if (epoll_ctl(_epollFd.getFd(), EPOLL_CTL_MOD, fd.getFd(), &event) == -1)
	{
		std::stringstream ss;
		ss << "epoll_ctl MOD failed for fd: " << fd.getFd() << ", errno: " << errno;
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

void EpollManager::removeFd(FileDescriptor fd)
{
	if (epoll_ctl(_epollFd.getFd(), EPOLL_CTL_DEL, fd.getFd(), NULL) == -1)
	{
		std::stringstream ss;
		ss << "epoll_ctl DEL failed for fd: " << fd;
		Logger::log(Logger::ERROR, ss.str());
	}
}

int EpollManager::wait(std::vector<epoll_event> &events, int timeout)
{
	if (events.empty())
	{
		events.resize(128); // Default buffer size
	}

	Logger::debug("EpollManager: Calling epoll_wait with timeout " + StrUtils::toString(timeout) + "ms");
	int readyCount = epoll_wait(_epollFd.getFd(), &events[0], events.size(), timeout);
	Logger::debug("EpollManager: epoll_wait returned " + StrUtils::toString(readyCount) + " events");
	
	if (readyCount == -1)
	{
		if (errno == EINTR)
		{
			// Interrupted by signal, this is normal during shutdown
			Logger::debug("EpollManager: epoll_wait interrupted by signal");
			return 0;
		}
		std::stringstream ss;
		ss << "epoll_wait failed: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	return readyCount;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/
FileDescriptor &EpollManager::getFd()
{
	return _epollFd;
}

/* ************************************************************************** */
