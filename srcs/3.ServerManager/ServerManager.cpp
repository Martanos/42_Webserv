#include "ServerManager.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ServerManager::ServerManager()
{
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

int ServerManager::_spawnPollingInstance()
{
	int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (epoll_fd == -1)
	{
		std::stringstream ss;
		ss << "ServerManager: Failed to create epoll instance: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	return epoll_fd;
}

void ServerManager::_addFdToEpoll(int fd)
{
	epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = fd;
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
	{
		std::stringstream ss;
		ss << "ServerManager: Failed to add fd to epoll: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

void ServerManager::_addServerFdsToEpoll(ServerMap &serverMap)
{
	for (std::map<ServerKey, std::vector<Server> >::const_iterator it = serverMap.getServerMap().begin(); it != serverMap.getServerMap().end(); ++it)
	{
		_addFdToEpoll(it->first.getFd());
	}
}

void ServerManager::_handleServerListening(int ready_events, ServerMap &serverMap, std::vector<epoll_event> &events)
{
	for (int i = 0; i < ready_events; ++i)
	{
		epoll_event &event = events[i];
		if (event.events & EPOLLIN && serverMap.hasFd(event.data.fd))
		{
			while (true)
			{
				struct sockaddr_in client_addr;
				socklen_t client_addr_len = sizeof(client_addr);
				int client_fd = accept(event.data.fd, (struct sockaddr *)&client_addr, &client_addr_len);
				if (client_fd == -1)
				{
					std::stringstream ss;
					ss << "ServerManager: Failed to accept client connection: " << strerror(errno);
					Logger::log(Logger::ERROR, ss.str());
					continue;
				}
				try
				{
					_addFdToEpoll(client_fd);
				}
				catch (const std::exception &e)
				{
					close(client_fd);
					continue;
				}
				Client client(client_fd, client_addr);
				std::pair<std::map<int, Client>::iterator, bool> insert_result = _clients.insert(std::make_pair(client_fd, client));
				if (!insert_result.second)
				{
					std::stringstream ss;
					ss << "ServerManager: Failed to insert client into map: " << strerror(errno);
					Logger::log(Logger::ERROR, ss.str());
					epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
					close(client_fd);
					continue;
				}
			}
		}
	}
}

void ServerManager::_handleClientConnections(int ready_events, ServerMap &serverMap, std::vector<epoll_event> &events)
{
	for (int i = 0; i < ready_events; ++i)
	{
		epoll_event &event = events[i];
		if (event.events & EPOLLIN && _clients.find(event.data.fd) != _clients.end())
		{
			_clients[event.data.fd].handleRead();
		}
	}
}

void ServerManager::run(std::vector<ServerConfig> &serverConfigs)
{
	// Spawn server map based on server configs
	_serverMap = ServerMap(serverConfigs);

	// Spawn epoll instance
	_epoll_fd = _spawnPollingInstance();

	// Add server fds to epoll
	_addServerFdsToEpoll(_serverMap);

	// Spawn a buffer for epoll events
	std::vector<epoll_event> events;
	events.reserve(128); // Arbitrary buffer size

	// main polling loop
	while (true)
	{
		// -1 denotes witing forever this may be a problem if the server is not ready to accept connections
		int ready_events = epoll_wait(_epoll_fd, events.data(), events.size(), -1);
		if (ready_events == -1)
		{
			// TODO: Check if this is the correct way to handle signals
			// if it is this may be where to mass send server shutdown signals
			if (errno == EINTR)
			{
				Logger::log(Logger::INFO, "Epoll wait interrupted by signal");
				continue;
			}
		}
		else if (ready_events == 0)
		{
			Logger::log(Logger::INFO, "Epoll wait returned 0, no events ready");
			continue;
		}
		else
		{
			_handleServerListening(ready_events, _serverMap, events);
			_handleClientConnections(ready_events, _serverMap, events);
		}
	}
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

/* ************************************************************************** */
