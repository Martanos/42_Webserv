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
	~ServerManager();

private:
	ServerManager(ServerManager const &src);
	ServerManager &operator=(ServerManager const &rhs);

	// Internal members
	ServerMap _serverMap;			// Map to servers via their host_port/connection fd
	std::map<int, Client> _clients; // clients that are currently active
	int _epoll_fd;					// epoll instance fd

private:
	// Private methods
	int _spawnPollingInstance();

	void _addServerFdsToEpoll(ServerMap &serverMap);
	void _addFdToEpoll(int fd);
	void _handleClientConnections(int ready_events, ServerMap &serverMap, std::vector<epoll_event> &events);
	void _handleServerListening(int ready_events, ServerMap &serverMap, std::vector<epoll_event> &events);

public:
	void run(std::vector<ServerConfig> &serverConfigs);
};

#endif /* *************************************************** SERVERMANAGER_H */
