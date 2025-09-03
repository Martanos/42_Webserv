#include "ServerManager.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ServerManager::ServerManager()
{
	_epollManager = EpollManager();
	_serverMap = ServerMap();
	_clients = std::map<FileDescriptor, Client>();
}

ServerManager::ServerManager(const ServerManager &src)
{
	throw std::runtime_error("ServerManager: Copy constructor called");
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ServerManager::~ServerManager()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ServerManager &ServerManager::operator=(ServerManager const &rhs)
{
	(void)rhs;
	throw std::runtime_error("ServerManager: Assignment operator called");
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void ServerManager::_addServerFdsToEpoll(ServerMap &serverMap)
{
	for (std::map<ListeningSocket, std::vector<Server> >::const_iterator it = serverMap.getServerMap().begin(); it != serverMap.getServerMap().end(); ++it)
	{
		_epollManager.addFd(it->first.getFd());
	}
}

void ServerManager::_handleEpollEvents(int ready_events, ServerMap &serverMap, std::vector<epoll_event> &events)
{
	for (int i = 0; i < ready_events; ++i)
	{
		int fd = events[i].data.fd;
		if (_serverMap.hasFd(fd))
		{
			for (std::map<ListeningSocket, std::vector<Server> >::const_iterator it = serverMap.getServerMap().begin(); it != serverMap.getServerMap().end(); ++it)
			{
				if (it->first.getFd() == fd)
				{
					FileDescriptor clientFd = it->first.accept();
					if (clientFd.isValid())
					{
						clientFd.setNonBlocking();
						SocketAddress clientAddr;
						Client newClient(clientFd, clientAddr);
						newClient.setServer(&(it->second[0]));
						_clients[clientFd] = newClient;
						_epollManager.addFd(clientFd.getFd());
						Logger::log(Logger::INFO, "New client connected: " + clientFd.getFd());
					}
					break;
				}
			}
		}
		else if (_clients.find(FileDescriptor(fd)) != _clients.end())
		{
			try
			{
				_clients[FileDescriptor(fd)].handleEvent(events[i]);
				if (_clients[FileDescriptor(fd)].getServer()->getKeepAlive() == false && _clients[FileDescriptor(fd)].getCurrentState() == Client::CLIENT_WAITING_FOR_REQUEST)
				{
					_clients.erase(FileDescriptor(fd));
					_epollManager.removeFd(fd);
					Logger::log(Logger::INFO, "Client disconnected: " + fd);
				}
			}
			catch (...)
			{
				_clients.erase(FileDescriptor(fd));
				_epollManager.removeFd(fd);
				Logger::log(Logger::INFO, "Client disconnected: " + fd);
			}
			continue;
		}
	}
}

void ServerManager::run(std::vector<ServerConfig> &serverConfigs)
{
	// TODO:SETUP SIGNAL HANDLING

	// Spawn server map based on server configs
	_serverMap = ServerMap(serverConfigs);

	// Spawn epoll instance
	_epollManager = EpollManager();

	// Add server fds to epoll
	_addServerFdsToEpoll(_serverMap);

	// Spawn a buffer for epoll events
	std::vector<epoll_event> events;

	// main polling loop
	while (true)
	{
		// TODO: -1 denotes witing forever this may be a problem if the server is not ready to accept connections
		int ready_events = _epollManager.wait(events, -1);
		if (ready_events == -1)
		{
			// TODO: Check if this is the correct way to handle signals
			if (errno == EINTR)
			{
				std::stringstream ss;
				ss << "Epoll wait interrupted by signal: " << strerror(errno);
				Logger::log(Logger::INFO, ss.str());
				throw std::runtime_error(ss.str());
			}
		}
		else if (ready_events == 0)
		{
			Logger::log(Logger::INFO, "Epoll wait returned 0, no events ready");
			continue;
		}
		else
			_handleEpollEvents(ready_events, _serverMap, events);
	}
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

/* ************************************************************************** */
