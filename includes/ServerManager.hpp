#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <iostream>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <queue>
#include <signal.h>
#include "Logger.hpp"
#include "ServerMap.hpp"
#include "Client.hpp"
#include "EpollManager.hpp"

// This class facilitates the main epoll loop and delegates to the appropriate classes to handle connections
class ServerManager
{
private:
	// Non-copyable
	ServerManager(ServerManager const &src);
	ServerManager &operator=(ServerManager const &rhs);

	bool serverRunning;

public:
	ServerManager();
	~ServerManager();

	// Internal members
	ServerMap _serverMap;					   // Map to servers via their host_port/connection fd
	std::map<FileDescriptor, Client> _clients; // clients that are currently active
	EpollManager _epollManager;				   // epoll instance class

private:
	void _addServerFdsToEpoll(ServerMap &serverMap);

	// Private methods

	void _handleEpollEvents(int ready_events, ServerMap &serverMap, std::vector<epoll_event> &events);

public:
	void run(std::vector<ServerConfig> &serverConfigs);
	static void _handleSignal(int signal);

	bool isServerRunning();
	static void setServerRunning(bool serverRunning);
};

#endif /* *************************************************** SERVERMANAGER_H */
