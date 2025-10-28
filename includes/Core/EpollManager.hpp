#ifndef EPOLLMANAGER_HPP
#define EPOLLMANAGER_HPP

#include "../../includes/Wrapper/FileDescriptor.hpp"
#include <cstring>
#include <sys/epoll.h>
#include <vector>

class EpollManager
{
private:
	FileDescriptor _epollFd;

	// Non-copyable
	EpollManager(const EpollManager &);

public:
	// Constructor
	EpollManager();
	~EpollManager();
	EpollManager &operator=(const EpollManager &);

	void addFd(FileDescriptor fd, uint32_t events = EPOLLIN);
	void modifyFd(FileDescriptor fd, uint32_t events);
	void removeFd(FileDescriptor fd);

	// Wait for events
	int wait(std::vector<epoll_event> &events, int timeout = -1);

	FileDescriptor &getFd();
};

#endif /* **************************************************** EPOLLMANAGER_H                                          \
		*/
