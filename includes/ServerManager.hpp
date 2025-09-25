#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "Client.hpp"
#include "EpollManager.hpp"
#include "ServerMap.hpp"
#include <map>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

// This class facilitates the main epoll loop and delegates to the appropriate
// classes to handle connections
class ServerManager
{
private:
	// Non-copyable
	ServerManager(ServerManager const &src);
	ServerManager &operator=(ServerManager const &rhs);

	static bool serverRunning;

public:
	ServerManager();
	~ServerManager();

	// Internal members
	ServerMap _serverMap;			// Map to servers via their host_port/connection fd
	std::map<int, Client> _clients; // clients that are currently active
	EpollManager _epollManager;		// epoll instance class

private:
	void _addServerFdsToEpoll(ServerMap &serverMap);
	void _checkClientTimeouts();

	// Private methods

	void _handleEpollEvents(int ready_events, std::vector<epoll_event> &events);

public:
	void run(std::vector<ServerConfig> &serverConfigs);
	static void _handleSignal(int signal);

	bool isServerRunning();
	static void setServerRunning(bool shouldRun);
};

#endif /* *************************************************** SERVERMANAGER_H                                          \
		*/
