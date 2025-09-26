#include "../../includes/ServerManager.hpp"
#include "../../includes/PerformanceMonitor.hpp"
#include "../../includes/StringUtils.hpp"

// Static member definition
bool ServerManager::serverRunning = true;

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ServerManager::ServerManager()
{
	_epollManager = EpollManager();
	_serverMap = ServerMap();
	_clients = std::map<int, Client>();
}

ServerManager::ServerManager(const ServerManager &src)
{
	(void)src;
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
	for (std::map<ListeningSocket, std::vector<Server> >::const_iterator it = serverMap.getServerMap().begin();
		 it != serverMap.getServerMap().end(); ++it)
	{
		_epollManager.addFd(it->first.getFd());
	}
	Logger::info("ServerManager: Added " + StringUtils::toString(serverMap.getServerMap().size()) +
				 " server file descriptors to epoll");
}

void ServerManager::_handleEpollEvents(int ready_events, std::vector<epoll_event> &events)
{
	for (int i = 0; i < ready_events; ++i)
	{
		int fd = events[i].data.fd;

		// TODO: Apply reference fix
		if (_serverMap.hasFd(fd))
		{
			const ListeningSocket &listeningSocket = _serverMap.getListeningSocket(fd);
			SocketAddress localAddr = listeningSocket.getAddress();
			SocketAddress remoteAddr;
			FileDescriptor clientFd;
			listeningSocket.accept(remoteAddr, clientFd);
			if (clientFd.getFd() != -1)
			{
				clientFd.setNonBlocking();
				Client newClient(clientFd, localAddr, remoteAddr);
				newClient.setPotentialServers(_serverMap.getServers(listeningSocket));
				_clients.insert(std::make_pair(newClient.getSocketFd(), newClient));
				_epollManager.addFd(newClient.getSocketFd(), EPOLLIN | EPOLLET);
				Logger::logClientConnect(remoteAddr.getHost(), remoteAddr.getPort());
				Logger::debug("ServerManager: Client " + StringUtils::toString(newClient.getSocketFd()) +
							  " added to epoll");

				// Record connection for performance monitoring
				PERF_RECORD_CONNECTION();
			}
		}
		else if (_clients.find(fd) != _clients.end())
		{
			try
			{
				Client::State oldState = _clients[fd].getCurrentState();
				_clients[fd].handleEvent(events[i]);
				Client::State newState = _clients[fd].getCurrentState();

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
				if (newState == Client::CLIENT_WAITING_FOR_REQUEST && !_clients[fd].getServer()->getKeepAlive())
				{
					_clients.erase(fd);
					_epollManager.removeFd(fd);
					std::stringstream ss;
					ss << "Client disconnected: " << fd;
					Logger::log(Logger::INFO, ss.str());
				}
				else if (newState == Client::CLIENT_CLOSING || newState == Client::CLIENT_DISCONNECTED)
				{
					// Client is closing or already disconnected, clean up
					_clients.erase(fd);
					_epollManager.removeFd(fd);
					std::stringstream ss;
					ss << "Client disconnected (state: " << newState << "): " << fd;
					Logger::log(Logger::INFO, ss.str());

					// Record disconnection for performance monitoring
					PERF_RECORD_DISCONNECTION();
				}
			}
			catch (const std::exception &e)
			{
				_clients.erase(fd);
				_epollManager.removeFd(fd);
				std::stringstream ss;
				ss << "Client " << fd << " disconnected: " << e.what();
				Logger::log(Logger::INFO, ss.str());

				// Record disconnection and error for performance monitoring
				PERF_RECORD_DISCONNECTION();
				PERF_RECORD_ERROR();
			}
		}
	}
}

void ServerManager::_checkClientTimeouts()
{
	std::vector<FileDescriptor> clientsToRemove;

	if (_clients.empty())
		return;

	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second.isTimedOut())
		{
			clientsToRemove.push_back(it->first);
		}
	}

	// Remove timed out clients
	for (std::vector<FileDescriptor>::iterator it = clientsToRemove.begin(); it != clientsToRemove.end(); ++it)
	{
		Logger::log(Logger::INFO, "Client " + StringUtils::toString(it->getFd()) + " timed out, disconnecting");
		_clients.erase(*it);
		_epollManager.removeFd(it->getFd());

		// Record timeout and disconnection for performance monitoring
		PERF_RECORD_TIMEOUT();
		PERF_RECORD_DISCONNECTION();
	}
}

void ServerManager::run(std::vector<ServerConfig> &serverConfigs)
{
	Logger::log(Logger::INFO, "Starting ServerManager with " + StringUtils::toString(serverConfigs.size()) +
								  " server configurations");

	// Spawn server map based on server configs
	_serverMap = ServerMap(serverConfigs);

	if (_serverMap.getServerMap().empty())
	{
		Logger::log(Logger::ERROR, "No valid server configurations found");
		return;
	}
	_serverMap.printServerMap();

	// Spawn epoll instance
	Logger::debug("ServerManager: Creating EpollManager");
	_epollManager = EpollManager();

	// Add server fds to epoll
	_addServerFdsToEpoll(_serverMap);

	// Spawn a buffer for epoll events
	std::vector<epoll_event> events;

	// Add a signal handler for SIGINT and SIGTERM
	signal(SIGINT, _handleSignal);
	signal(SIGTERM, _handleSignal);

	Logger::log(Logger::INFO, "ServerManager initialized successfully, entering main event loop");

	// main polling loop
	while (isServerRunning())
	{
		int ready_events = _epollManager.wait(events, 1000); // 1 second timeout
		if (ready_events == -1)
		{
			Logger::logErrno(Logger::ERROR, "Epoll wait failed", __FILE__, __LINE__);
			break;
		}
		else if (ready_events == 0)
		{
			// Timeout - check for client timeouts
			_checkClientTimeouts();
			continue;
		}
		else
		{
			// Record epoll events for performance monitoring
			PERF_RECORD_EPOLL_EVENT();
			_handleEpollEvents(ready_events, events);
		}
	}

	Logger::log(Logger::INFO, "ServerManager shutting down");
}

void ServerManager::_handleSignal(int signal)
{
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

void ServerManager::setServerRunning(bool shouldRun)
{
	serverRunning = shouldRun;
}

/* ************************************************************************** */
