#include "../../includes/Core/ServerManager.hpp"
#include "../../includes/Global/PerformanceMonitor.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/PerformanceMonitor.hpp"
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
	for (std::map<ListeningSocket, std::vector<Server> >::const_iterator it = serverMap.getServerMap().begin();
		 it != serverMap.getServerMap().end(); ++it)
	{
		_epollManager.addFd(it->first.getFd());
	}
	Logger::debug("ServerManager: Added " + StrUtils::toString(serverMap.getServerMap().size()) +
				  " server file descriptors to epoll");
}

void ServerManager::_handleEventLoop(int ready_events, std::vector<epoll_event> &events)
{
	for (int i = 0; i < ready_events; ++i)
	{
		int fd = events[i].data.fd;

		if (_serverMap.hasFd(fd))
		{
			try
			{
				const ListeningSocket &listeningSocket = ;
				SocketAddress localAddr = listeningSocket.getAddress();
				SocketAddress remoteAddr;
				FileDescriptor clientFd;
				listeningSocket.accept(remoteAddr, clientFd);
				if (clientFd.getFd() != -1)
				{
					Client newClient(clientFd, localAddr, remoteAddr);
					newClient.setPotentialServers(_serverMap.getServers(listeningSocket));
					_clients.insert(std::make_pair(clientFd, newClient));
					_epollManager.addFd(clientFd, EPOLLIN | EPOLLET);
					Logger::logClientConnect(remoteAddr.getHost(), remoteAddr.getPort());
					Logger::debug("ServerManager: Client " + StrUtils::toString(clientFd.getFd()) + " added to epoll");

					// Record connection for performance monitoring
					PERF_RECORD_CONNECTION();
				}
			}
			catch (const std::exception &e)
			{
				Logger::log(Logger::ERROR, "ServerManager: Error accepting client: " + std::string(e.what()));
			}
		}
		else
		{
			FileDescriptor foundFd;
			for (std::map<FileDescriptor, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
			{
				if (it->first.getFd() == fd)
				{
					foundFd = it->first;
					break;
				}
			}
			if (!foundFd.isValid())
			{
				continue;
			}
			try
			{
				Client::ClientState oldState = _clients[foundFd].getCurrentState();
				_clients[foundFd].handleEvent(events[i]);
				Client::ClientState newState = _clients[foundFd].getCurrentState();

				// Update epoll events based on state change
				if (oldState != newState)
				{
					uint32_t epollEvents = 0;
					switch (newState)
					{
					case Client::CLIENT_PROCESSING_RESPONSES:
						epollEvents = EPOLLOUT | EPOLLET;
						break;
					case Client::CLIENT_DISCONNECTED:
					{
						// Client is closing or already disconnected, clean up
						_clients.erase(foundFd);
						epollEvents = EPOLLHUP | EPOLLRDHUP | EPOLLERR;
						Logger::log(Logger::INFO, "Client " + StrUtils::toString(fd) +
													  " disconnected (state: " + StrUtils::toString(newState) + ")");
						PERF_RECORD_DISCONNECTION();
						break;
					}
					case Client::CLIENT_WAITING_FOR_REQUEST:
					{
						switch (_clients[foundFd].getServer()->getKeepAlive())
						{
						case true:
							epollEvents = EPOLLIN | EPOLLET;
							break;
						case false:
							epollEvents = EPOLLHUP | EPOLLRDHUP | EPOLLERR;
							break;
						}
					}
					default:
						epollEvents = EPOLLIN | EPOLLET;
						break;
					}
					_epollManager.modifyFd(foundFd, epollEvents);
				}
			}
			catch (const std::exception &e)
			{
				_clients.erase(foundFd);
				_epollManager.removeFd(foundFd);
				Logger::log(Logger::ERROR, "ServerManager: Error handling client event: " + std::string(e.what()),
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
	}
}

void ServerManager::_checkClientTimeouts()
{
	std::vector<FileDescriptor> clientsToRemove;

	if (_clients.empty())
		return;

	for (std::map<FileDescriptor, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second.isTimedOut())
		{
			clientsToRemove.push_back(it->first);
		}
	}

	// Remove timed out clients
	for (std::vector<FileDescriptor>::iterator it = clientsToRemove.begin(); it != clientsToRemove.end(); ++it)
	{
		Logger::log(Logger::INFO, "Client " + StrUtils::toString(it->getFd()) + " timed out, disconnecting");
		_clients.erase(*it);
		_epollManager.removeFd(*it);

		// Record timeout and disconnection for performance monitoring
		PERF_RECORD_TIMEOUT();
		PERF_RECORD_DISCONNECTION();
	}
}

void ServerManager::run()
{
	Logger::log(Logger::INFO,
				"Starting ServerManager with " + StrUtils::toString(_serverMap.getServerMap().size()) + " servers");

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
		int ready_events = _epollManager.wait(events, 30);
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
			_handleEventLoop(ready_events, events);
		}
	}

	Logger::log(Logger::INFO, "ServerManager shutting down");
}

void ServerManager::_handleSignal(int signal)
{
	setServerRunning(false);
	Logger::log(Logger::INFO, "Signal received: " + StrUtils::toString(signal));
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
