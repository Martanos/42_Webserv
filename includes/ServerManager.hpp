#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <iostream>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <queue>
#include "Logger.hpp"
#include "ServerMap.hpp"
#include "Client.hpp"

// This class facilitates the main epoll loop and delegates to the appropriate classes to handle connections
class ServerManager
{
public:
	ServerManager();
	ServerManager(ServerManager const &src);
	~ServerManager();
	ServerManager &operator=(ServerManager const &rhs);

private:
	// Internal members
	ServerMap _serverMap; // Map to servers via their host_port/connection fd
	// TODO: create a client manager class to handle client connections
	std::map<int, Client> _clients; // clients that are currently active
	int _epoll_fd;					// epoll instance fd

private:
	// Private methods
	int _spawnPollingInstance(ServerMap &serverMap)
	{
		int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
		if (epoll_fd == -1)
		{
			Logger::log(Logger::ERROR, "Failed to create epoll instance");
			throw std::runtime_error("Failed to create epoll instance");
		}
		{
			std::stringstream ss;
			ss << "Epoll instance created with fd: " << epoll_fd;
			Logger::log(Logger::INFO, ss.str());
		}
		return epoll_fd;
	}

	void _addServerFdsToEpoll(int epoll_fd, ServerMap &serverMap)
	{
		std::map<int, std::pair<std::string, unsigned short> > fd_host_port_map = serverMap.getFd_host_port_map();
		for (std::map<int, std::pair<std::string, unsigned short> >::iterator it = fd_host_port_map.begin(); it != fd_host_port_map.end(); ++it)
		{
			epoll_event event;
			event.events = EPOLLIN;
			event.data.fd = it->first;
			epoll_ctl(epoll_fd, EPOLL_CTL_ADD, it->first, &event);
		}
	}

public:
	void
	run(std::vector<ServerConfig> &serverConfigs)
	{
		// Spawn server map based on server configs
		_serverMap = ServerMap(serverConfigs);

		// Spawn epoll instance
		_epoll_fd = _spawnPollingInstance(_serverMap);

		// Add server fds to epoll
		_addServerFdsToEpoll(_epoll_fd, _serverMap);

		// Spawn a buffer for client connections
		std::vector<Client> clients(64);

		// Spawn a buffer for epoll events
		int buffer_size = 128; // Arbitrary buffer size
		std::vector<epoll_event> events(buffer_size);

		// main polling loop
		while (true)
		{
			// -1 denotes witing forever this may be a problem if the server is not ready to accept connections
			int ready_events = epoll_wait(_epoll_fd, events.data(), events.size(), -1);
			if (ready_events == -1)
			{
				// TODO: Check if this is the correct way to handle signals
				// if it is this may be where to mass send server shutdown signals
				if (errno == EINTR)
				{
					Logger::log(Logger::INFO, "Epoll wait interrupted by signal");
					continue;
				}
			}
			else if (ready_events == 0)
			{
				Logger::log(Logger::INFO, "Epoll wait returned 0, no events ready");
				continue;
			}
			else
			{
				for (int i = 0; i < ready_events; i++)
				{
					epoll_event &event = events[i];
				}
			}
		}
	}
}
}
;

#endif /* *************************************************** SERVERMANAGER_H */
