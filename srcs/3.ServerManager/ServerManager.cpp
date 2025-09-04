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
			// Handle new connection
			for (std::map<ListeningSocket, std::vector<Server> >::const_iterator it = serverMap.getServerMap().begin();
				 it != serverMap.getServerMap().end(); ++it)
			{
				if (it->first.getFd() == fd)
				{
					FileDescriptor clientFd = it->first.accept();
					if (clientFd.isValid())
					{
						clientFd.setNonBlocking();
						SocketAddress clientAddr;
						Client newClient(clientFd, clientAddr);
						newClient.setPotentialServers(serverMap.getServers(const_cast<ListeningSocket &>(it->first)));
						_epollManager.addFd(clientFd.getFd(), EPOLLIN | EPOLLET);

						std::stringstream ss;
						ss << "New client connected: " << clientFd.getFd();
						Logger::log(Logger::INFO, ss.str());
					}
					break;
				}
			}
		}
		else if (_clients.find(FileDescriptor(fd)) != _clients.end())
		{
			try
			{
				Client::State oldState = _clients[FileDescriptor(fd)].getCurrentState();
				_clients[FileDescriptor(fd)].handleEvent(events[i]);
				Client::State newState = _clients[FileDescriptor(fd)].getCurrentState();

				// Update epoll events based on state change
				if (oldState != newState)
				{
					uint32_t epollEvents = 0;
					switch (newState)
					{
					case Client::CLIENT_READING_REQUEST:
					case Client::CLIENT_READING_FILE:
						epollEvents = EPOLLIN | EPOLLET;
						break;
					case Client::CLIENT_SENDING_RESPONSE:
						epollEvents = EPOLLOUT | EPOLLET;
						break;
					case Client::CLIENT_WAITING_FOR_REQUEST:
						epollEvents = EPOLLIN | EPOLLET;
						break;
					default:
						epollEvents = EPOLLIN | EPOLLET;
						break;
					}
					_epollManager.modifyFd(fd, epollEvents);
				}

				// Check for connection cleanup
				if (newState == Client::CLIENT_WAITING_FOR_REQUEST && !_clients[FileDescriptor(fd)].getServer()->getKeepAlive())
				{
					_clients.erase(FileDescriptor(fd));
					_epollManager.removeFd(fd);
					std::stringstream ss;
					ss << "Client disconnected: " << fd;
					Logger::log(Logger::INFO, ss.str());
				}
			}
			catch (const std::exception &e)
			{
				_clients.erase(FileDescriptor(fd));
				_epollManager.removeFd(fd);
				std::stringstream ss;
				ss << "Client " << fd << " disconnected: " << e.what();
				Logger::log(Logger::INFO, ss.str());
			}
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

	// Add a signal handler for SIGINT and SIGTERM
	signal(SIGINT, ServerManager::_handleSignal);
	signal(SIGTERM, ServerManager::_handleSignal);

	// main polling loop
	while (isServerRunning())
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

void ServerManager::_handleSignal(int signal)
{
	(void)signal;
	setServerRunning(false);
	Logger::log(Logger::INFO, "Signal received: " + StringUtils::toString(signal));
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

bool ServerManager::isServerRunning()
{
	return serverRunning;
}

void ServerManager::setServerRunning(bool serverRunning)
{
	serverRunning = serverRunning;
}

/* ************************************************************************** */
