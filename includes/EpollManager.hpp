#ifndef EPOLLMANAGER_HPP
#define EPOLLMANAGER_HPP

#include <iostream>
#include <string>
#include <vector>

#include "FileDescriptor.hpp"
#include "Logger.hpp"

class EpollManager
{
private:
	FileDescriptor _epollFd;

	// Non-copyable
	EpollManager(const EpollManager &);
	EpollManager &operator=(const EpollManager &);

public:
	// Constructor
	EpollManager();

	void addFd(int fd, uint32_t events = EPOLLIN);
	void modifyFd(int fd, uint32_t events);
	void removeFd(int fd);

	// Wait for events
	int wait(std::vector<epoll_event> &events, int timeout = -1);

	int getFd() const;
};

#endif /* **************************************************** EPOLLMANAGER_H */
