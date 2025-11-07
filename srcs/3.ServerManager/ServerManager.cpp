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
	_clients = std::map<int, Client>();
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
		_epollManager.addFd(it->first.getFd().getFd());
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
		else if (_clients.find(fd) != _clients.end())
		{
			// Handle existing client
			Logger::debug("ServerManager: This is a client fd, handling client event");
			_handleClientEvent(_clients.find(fd)->second, events[i]);
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
	SocketAddress remoteAddress;
	FileDescriptor clientFdObj = FileDescriptor::createFromAccept(serverFd, remoteAddress);
	if (!clientFdObj.isValid())
		return Logger::error("ServerManager: Failed to accept connection: " + std::string(strerror(errno)), __FILE__,
							 __LINE__, __PRETTY_FUNCTION__);
	else if (!clientFdObj.setNonBlocking())
		return Logger::error("ServerManager: Failed to set socket to non-blocking: " + std::string(strerror(errno)),
							 __FILE__, __LINE__, __PRETTY_FUNCTION__);
	// Create client object
	Client client(clientFdObj, remoteAddress);
	// Set potential servers for this client
	client.setPotentialServers(_serverMap.getServersForFd(serverFd));
	// Add client to epoll
	_epollManager.addFd(client.getSocketFd(), EPOLLIN);
	// Store client in map
	_clients[client.getSocketFd()] = client;
	Logger::debug("ServerManager: New client connected from " + remoteAddress.getHostString() + ":" +
				  remoteAddress.getPortString() + " to server fd: " + StrUtils::toString(serverFd));
}

void ServerManager::_handleClientEvent(Client &client, epoll_event event)
{
	// Pass intial event to client
	Logger::debug("ServerManager: Handling client event for client: " + client.getRemoteAddr().getHostString() + ":" +
				  client.getRemoteAddr().getPortString());
	client.handleEvent(event);
	switch (client.getCurrentState())
	{
	case Client::WAITING_FOR_EPOLLIN:
	{
		Logger::debug("ServerManager: Client is waiting for EPOLLIN, modifying epoll for EPOLLIN");
		_epollManager.modifyFd(client.getSocketFd(), EPOLLIN);
		break;
	}
	case Client::WAITING_FOR_EPOLLOUT:
	{
		Logger::debug("ServerManager: Client is waiting for EPOLLOUT, modifying epoll for EPOLLOUT");
		_epollManager.modifyFd(client.getSocketFd(), EPOLLOUT);
		break;
	}
	case Client::DISCONNECTED:
	{
		Logger::debug("ServerManager: Client is disconnected, removing from epoll and clients map");
		_epollManager.removeFd(client.getSocketFd());
		_clients.erase(client.getSocketFd());
		break;
	}
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
		std::vector<int> timedOutClients;
		for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second.isTimedOut())
			{
				timedOutClients.push_back(it->first);
			}
		}
		for (std::vector<int>::iterator it = timedOutClients.begin(); it != timedOutClients.end(); ++it)
		{
			Logger::debug("ServerManager: Client timed out, removing from epoll and clients map");
			Logger::debug("ServerManager: Client fd: " + StrUtils::toString(*it));
			_epollManager.removeFd(*it);
			_clients.erase(*it);
		}
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