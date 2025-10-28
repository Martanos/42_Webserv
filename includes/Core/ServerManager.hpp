#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "../../includes/ConfigParser/ServerMap.hpp"
#include "../../includes/Core/Client.hpp"
#include "../../includes/Core/EpollManager.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Wrapper/FileDescriptor.hpp"
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

	void _addServerFdsToEpoll(ServerMap &serverMap);
	void _checkClientTimeouts();

	// Private methods
	void _handleEventLoop(int ready_events, std::vector<epoll_event> &events);

	// Signal handlers
	static void _handleSignal(int signal);

	// Internal members
	ServerMap _serverMap;					   // Map to servers via their host_port/connection fd
	std::map<FileDescriptor, Client> _clients; // clients that are currently active
	EpollManager _epollManager;				   // epoll instance class

public:
	explicit ServerManager(ServerMap &serverMap);
	~ServerManager();

	void run();

	bool isServerRunning();
	static void setServerRunning(bool shouldRun);
};

#endif /* *************************************************** SERVERMANAGER_H                                          \
		*/
