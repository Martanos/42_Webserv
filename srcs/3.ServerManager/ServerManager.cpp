#include "../../includes/Core/ServerManager.hpp"
#include "../../includes/Global/PerformanceMonitor.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include <signal.h>

// Static member definition
bool ServerManager::serverRunning = true;

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ServerManager::ServerManager(ServerMap &serverMap) : _serverMap(serverMap)
{
	_epollManager = EpollManager();
	_clients = std::map<FileDescriptor, Client>();
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ServerManager::~ServerManager()
{
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void ServerManager::_addServerFdsToEpoll(ServerMap &serverMap)
{
	Logger::debug("ServerManager: Adding server FDs to epoll, serverMap size: " +
				  StrUtils::toString(serverMap.getServerMap().size()));

	for (std::map<ListeningSocket, std::vector<Server> >::const_iterator it = serverMap.getServerMap().begin();
		 it != serverMap.getServerMap().end(); ++it)
	{
		Logger::debug("ServerManager: Adding server fd: " + StrUtils::toString(it->first.getFd().getFd()) +
					  " to epoll");
		_epollManager.addFd(it->first.getFd());
	}
	Logger::debug("ServerManager: Added " + StrUtils::toString(serverMap.getServerMap().size()) +
				  " server file descriptors to epoll");
}

void ServerManager::_handleEventLoop(int ready_events, std::vector<epoll_event> &events)
{
	Logger::debug("ServerManager: Handling " + StrUtils::toString(ready_events) + " events");

	for (int i = 0; i < ready_events; ++i)
	{
		int fd = events[i].data.fd;
		Logger::debug("ServerManager: Processing event for fd: " + StrUtils::toString(fd) +
					  ", events: " + StrUtils::toString(events[i].events));

		if (_serverMap.hasFd(fd))
		{
			// Handle new connection
			Logger::debug("ServerManager: This is a server fd, handling new connection");
			Logger::debug("ServerManager: Server fd: " + StrUtils::toString(fd) +
						  ", events: " + StrUtils::toString(events[i].events));
			_handleNewConnection(fd);
		}
		else if (_isClientFd(fd))
		{
			// Handle existing client
			Logger::debug("ServerManager: This is a client fd, handling client event");
			_handleClientEvent(fd, events[i]);
		}
		else
		{
			Logger::debug("ServerManager: Unknown fd: " + StrUtils::toString(fd));
		}
	}
}

void ServerManager::_handleNewConnection(int serverFd)
{
	Logger::debug("ServerManager: Handling new connection on server fd: " + StrUtils::toString(serverFd));

	// Accept new connection
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
	Logger::debug("ServerManager: Accepted client fd: " + StrUtils::toString(clientFd));

	if (clientFd == -1)
	{
		Logger::error("ServerManager: Failed to accept connection");
		return;
	}

	// Set socket to non-blocking
	int flags = fcntl(clientFd, F_GETFL, 0);
	if (flags == -1 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		Logger::error("ServerManager: Failed to set socket to non-blocking");
		close(clientFd);
		return;
	}

	// Create client object
	SocketAddress localAddr;
	SocketAddress remoteAddr;
	FileDescriptor clientFdObj = FileDescriptor::createFromDup(clientFd);
	Client client(clientFdObj, localAddr, remoteAddr);

	// Set potential servers for this client
	const std::vector<Server> &potentialServers = _serverMap.getServersForFd(serverFd);
	client.setPotentialServers(potentialServers);
	printf("ServerManager: Potential servers: %zu\n", potentialServers.size());
	for (std::vector<Server>::const_iterator server = potentialServers.begin(); server != potentialServers.end();
		 ++server)
	{
		for (TrieTree<std::string>::const_iterator serverName = server->getServerNames().begin();
			 serverName != server->getServerNames().end(); ++serverName)
		{
			printf("ServerManager: Server name: %s\n", serverName->c_str());
		}
	}

	// Add client to epoll
	_epollManager.addFd(clientFdObj);
	_clients[clientFdObj] = client;

	Logger::debug("ServerManager: New client connected, fd: " + StrUtils::toString(clientFd));
}

void ServerManager::_handleClientEvent(int clientFd, epoll_event event)
{
	std::map<FileDescriptor, Client>::iterator it = _clients.begin();
	for (; it != _clients.end(); ++it)
	{
		if (it->first == clientFd)
		{
			break;
		}
	}

	if (it == _clients.end())
		return;

	Client &client = it->second;

	// Check if client is disconnected
	if (client.getCurrentState() == Client::CLIENT_DISCONNECTED)
	{
		_epollManager.removeFd(it->first);
		_clients.erase(it);
		Logger::debug("ServerManager: Client disconnected, fd: " + StrUtils::toString(clientFd));
		return;
	}

	if (event.events & EPOLLIN)
	{
		// Client has data to read
		client.handleEvent(event);
	}
	else if (event.events & EPOLLOUT)
	{
		// Client is ready to receive data
		client.handleEvent(event);
	}
	else if (event.events & (EPOLLERR | EPOLLHUP))
	{
		// Client disconnected or error occurred
		_epollManager.removeFd(it->first);
		_clients.erase(it);
		Logger::debug("ServerManager: Client disconnected, fd: " + StrUtils::toString(clientFd));
		return;
	}

	// Check client state after handling event
	if (client.getCurrentState() == Client::CLIENT_DISCONNECTED)
	{
		_epollManager.removeFd(it->first);
		_clients.erase(it);
		Logger::debug("ServerManager: Client disconnected after event, fd: " + StrUtils::toString(clientFd));
	}
	else if (client.getCurrentState() == Client::CLIENT_PROCESSING_RESPONSES)
	{
		// Client has response ready, modify epoll to watch for write events
		Logger::debug("ServerManager: Client has response ready, modifying epoll for EPOLLOUT");
		_epollManager.modifyFd(it->first, EPOLLOUT);
	}
}

bool ServerManager::_isClientFd(int fd) const
{
	for (std::map<FileDescriptor, Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->first == fd)
		{
			return true;
		}
	}
	return false;
}

void ServerManager::_checkClientTimeouts()
{
	std::vector<FileDescriptor> timedOutClients;

	for (std::map<FileDescriptor, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second.isTimedOut())
		{
			timedOutClients.push_back(it->first);
		}
	}

	for (std::vector<FileDescriptor>::iterator it = timedOutClients.begin(); it != timedOutClients.end(); ++it)
	{
		_epollManager.removeFd(*it);
		_clients.erase(*it);
		Logger::debug("ServerManager: Client timed out, fd: " + StrUtils::toString(it->getFd()));
	}
}

void ServerManager::run()
{
	Logger::info("ServerManager: Starting server manager");

	if (_serverMap.empty())
		throw std::runtime_error("ServerManager: No servers to run");
	_serverMap.printServerMap();
	// Add server file descriptors to epoll
	_addServerFdsToEpoll(_serverMap);

	// Set up signal handlers
	signal(SIGINT, _handleSignal);
	signal(SIGTERM, _handleSignal);

	// Main event loop
	while (serverRunning)
	{
		std::vector<epoll_event> events(100);				 // Max 100 events per iteration
		int ready_events = _epollManager.wait(events, 1000); // 1 second timeout
		if (ready_events > 0)
		{
			_handleEventLoop(ready_events, events);
		}

		// Check for timed out clients
		_checkClientTimeouts();
	}

	Logger::info("ServerManager: Server manager stopped");
}

bool ServerManager::isServerRunning()
{
	return serverRunning;
}

void ServerManager::setServerRunning(bool shouldRun)
{
	serverRunning = shouldRun;
}

void ServerManager::_handleSignal(int signal)
{
	(void)signal;
	Logger::info("ServerManager: Received signal, shutting down gracefully");
	serverRunning = false;
}