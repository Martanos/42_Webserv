#include "../../includes/Wrappers/EpollManager.hpp"
#include "../../includes/StringUtils.hpp"

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
		Logger::error(ss.str(), __FILE__, __LINE__);
		throw std::runtime_error(ss.str());
	}
	Logger::info("EpollManager: Epoll instance created successfully with fd " +
				 StringUtils::toString(_epollFd.getFd()));
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

	if (epoll_ctl(_epollFd.getFd(), EPOLL_CTL_ADD, fd.getFd(), &event) == -1)
	{
		std::stringstream ss;
		ss << "epoll_ctl ADD failed for fd: " << fd.getFd();
		Logger::error(ss.str(), __FILE__, __LINE__);
		throw std::runtime_error(ss.str());
	}
}

void EpollManager::modifyFd(FileDescriptor fd, uint32_t events)
{
	epoll_event event;
	event.events = events;
	event.data.fd = fd.getFd();

	if (epoll_ctl(_epollFd.getFd(), EPOLL_CTL_MOD, fd.getFd(), &event) == -1)
	{
		std::stringstream ss;
		ss << "epoll_ctl MOD failed for fd: " << fd;
		Logger::error(ss.str(), __FILE__, __LINE__);
		throw std::runtime_error(ss.str());
	}
}

void EpollManager::removeFd(FileDescriptor fd)
{
	if (epoll_ctl(_epollFd.getFd(), EPOLL_CTL_DEL, fd.getFd(), NULL) == -1)
	{
		std::stringstream ss;
		ss << "epoll_ctl DEL failed for fd: " << fd;
		Logger::error(ss.str(), __FILE__, __LINE__);
	}
}

int EpollManager::wait(std::vector<epoll_event> &events, int timeout)
{
	if (events.empty())
	{
		events.resize(128); // Default buffer size
	}

	int readyCount = epoll_wait(_epollFd.getFd(), &events[0], events.size(), timeout);
	if (readyCount == -1)
	{
		std::stringstream ss;
		ss << "epoll_wait failed: " << strerror(errno);
		Logger::error(ss.str(), __FILE__, __LINE__);
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
