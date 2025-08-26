#include "../../includes/ServerMap.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ServerMap::ServerMap()
{
	_fd_host_port_map = std::map<int, std::pair<std::string, unsigned short> >();
	_serverMap = std::map<std::pair<std::string, unsigned short>, std::vector<Server> >();
}

ServerMap::ServerMap(const ServerMap &src)
{
	_fd_host_port_map = src._fd_host_port_map;
	_serverMap = src._serverMap;
}

ServerMap::ServerMap(std::vector<ServerConfig> &serverConfigs)
{
	_spawnServerMap(serverConfigs);
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ServerMap::~ServerMap()
{
	for (std::map<int, std::pair<std::string, unsigned short> >::iterator it = _fd_host_port_map.begin(); it != _fd_host_port_map.end(); ++it)
		close(it->first);
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ServerMap &ServerMap::operator=(ServerMap const &rhs)
{
	if (this != &rhs)
	{
		_fd_host_port_map = rhs._fd_host_port_map;
		_serverMap = rhs._serverMap;
	}
	return *this;
}

/*
** --------------------------------- MAIN METHODS ----------------------------------
*/

void ServerMap::_spawnServerMap(std::vector<ServerConfig> &serverConfigs)
{
	std::vector<Server> servers = _spawnServers(serverConfigs);
	_spawnServerKeys(servers);
	_populateServerMap(servers);
}

/*
** --------------------------------- UTILITY METHODS ----------------------------------
*/

// Responsible for spawning the servers from the server configs server class will take care of spawning the locations
std::vector<Server> ServerMap::_spawnServers(std::vector<ServerConfig> &serverConfigs)
{
	std::vector<Server> servers;
	// Small optimisation for memory space
	servers.reserve(serverConfigs.size() * 2);
	// Iterate through each server config
	for (std::vector<ServerConfig>::iterator it = serverConfigs.begin(); it != serverConfigs.end(); ++it)
	{
		// Iterate through each server name
		for (std::vector<std::string>::const_iterator serverName = it->getServerNames().begin(); serverName != it->getServerNames().end(); ++serverName)
		{
			// Iterate through each host + port pair
			for (std::vector<std::pair<std::string, unsigned short> >::const_iterator host_port = it->getHosts_ports().begin(); host_port != it->getHosts_ports().end(); ++host_port)
			{
				// Spawn a server
				servers.push_back(Server(*serverName, *host_port, *it));
			}
		}
	}
	return servers;
}

// This methods populates the unbinded server keys map with the potential server keys
void ServerMap::_spawnServerKeys(std::vector<Server> &servers)
{
	// Populate _serverMap first
	for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
	{
		if (_serverMap.find(it->getHost_port()) == _serverMap.end())
			_serverMap[it->getHost_port()] = std::vector<Server>();
	}

	// Attempt to bind the server keys
	for (std::map<std::pair<std::string, unsigned short>, std::vector<Server> >::iterator it = _serverMap.begin(); it != _serverMap.end(); ++it)
	{
		if (_fd_host_port_map.size() >= FD_SETSIZE)
		{
			std::stringstream ss;
			ss << "ServerMap: Max number of server keys reached, FD_SETSIZE: " << FD_SETSIZE;
			Logger::log(Logger::INFO, ss.str());
			break;
		}

		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd == -1)
		{
			std::stringstream ss;
			ss << "ServerMap: Failed to create socket for " << it->first.first << ":" << it->first.second << " errno: " << strerror(errno);
			Logger::log(Logger::ERROR, ss.str());
			continue;
		}

		// Set socket options
		int opt = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		{
			std::stringstream ss;
			ss << "ServerMap: Failed to set SO_REUSEADDR for " << it->first.first << ":" << it->first.second;
			Logger::log(Logger::WARNING, ss.str());
		}

		// Make socket non-blocking
		if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		{
			std::stringstream ss;
			ss << "ServerMap: Failed to make socket non-blocking for " << it->first.first << ":" << it->first.second;
			Logger::log(Logger::ERROR, ss.str());
			close(fd);
			continue;
		}

		// Setup sockaddr_in structure
		struct sockaddr_in server_addr;
		std::memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(it->first.second);

		// If the host is 0.0.0.0 or empty, bind to INADDR_ANY effectively binding to all interfaces
		if (it->first.first == "0.0.0.0" || it->first.first.empty())
			server_addr.sin_addr.s_addr = INADDR_ANY;

		// Bind socket
		if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
		{
			std::stringstream ss;
			ss << "ServerMap: Failed to bind " << it->first.first << ":" << it->first.second << " - " << strerror(errno);
			Logger::log(Logger::ERROR, ss.str());
			close(fd);
			continue;
		}

		// Listen on socket
		if (listen(fd, 1024) == -1)
		{
			std::stringstream ss;
			ss << "ServerMap: Failed to listen on " << it->first.first << ":" << it->first.second << " - " << strerror(errno);
			Logger::log(Logger::ERROR, ss.str());
			close(fd);
			continue;
		}

		// Success - add to map
		_fd_host_port_map[fd] = it->first;
		{
			std::stringstream ss;
			ss << "ServerMap: Successfully bound " << it->first.first << ":" << it->first.second << " to fd " << fd;
			Logger::log(Logger::INFO, ss.str());
		}
	}

	if (_fd_host_port_map.empty())
	{
		std::string error = "ServerMap: No server sockets could be bound";
		Logger::log(Logger::ERROR, error);
		throw std::runtime_error(error);
	}
}

void ServerMap::_populateServerMap(std::vector<Server> &servers)
{
	// Iterate through each server
	for (std::vector<Server>::iterator server_object_it = servers.begin(); server_object_it != servers.end(); ++server_object_it)
	{
		// Attempt to match the server object o a key in the server map
		for (std::map<std::pair<std::string, unsigned short>, std::vector<Server> >::iterator it = _serverMap.begin(); it != _serverMap.end(); ++it)
		{
			// If the server object matches the key
			if (it->first == server_object_it->getHost_port())
			{
				// Check that the vector does not contain a similar server object
				for (std::vector<Server>::iterator server = it->second.begin(); server != it->second.end(); ++server)
				{
					// If it does then ignore the current server object and continue to the next server object
					if (server->getServerName() == server_object_it->getServerName())
					{
						std::stringstream ss;
						ss << "ServerMap: Server " << server_object_it->getServerName() << " already exists in server map, ignoring this server object: " << *server_object_it;
						Logger::log(Logger::ERROR, ss.str());
						break;
					}
				}
				// Add the server object to the server map
				it->second.push_back(*server_object_it);
			}
		}
	}
}

/*
** --------------------------------- ACCESSORS ----------------------------------
*/

const Server &ServerMap::getServer(const std::string &host, const unsigned short &port, const std::string &serverName)
{
	for (std::map<std::pair<std::string, unsigned short>, std::vector<Server> >::iterator it = _serverMap.begin(); it != _serverMap.end(); ++it)
	{
		for (std::vector<Server>::iterator server = it->second.begin(); server != it->second.end(); ++server)
		{
			if (server->getServerName() == serverName && server->getHost_port().first == host && server->getHost_port().second == port)
				return *server;
		}
	}
	return Server();
}

const Server &ServerMap::getServer(std::string &host, unsigned short &port, std::string &serverName)
{
	return getServer(static_cast<const std::string>(host), static_cast<const unsigned short>(port), static_cast<const std::string>(serverName));
}

const Server &ServerMap::getServer(const int &fd, const std::string &serverName)
{
	std::pair<std::string, unsigned short> host_port;
	// Find the corresponding host:port pair
	for (std::map<int, std::pair<std::string, unsigned short> >::iterator it = _fd_host_port_map.begin(); it != _fd_host_port_map.end(); ++it)
	{
		if (it->first == fd)
			host_port = it->second;
	}
	if (host_port.first.empty() || host_port.second == 0)
		return Server();
	for (std::vector<Server>::iterator server = _serverMap[host_port].begin(); server != _serverMap[host_port].end(); ++server)
	{
		if (server->getServerName() == serverName)
			return *server;
	}
	return Server();
}

const Server &ServerMap::getServer(int &fd, std::string &serverName)
{
	return getServer(static_cast<const int>(fd), static_cast<const std::string>(serverName));
}

bool ServerMap::hasFd(int &fd)
{
	return _fd_host_port_map.find(fd) != _fd_host_port_map.end();
}

/* ************************************************************************** */
