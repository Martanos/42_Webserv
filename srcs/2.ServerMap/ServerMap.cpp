#include "../../includes/ServerMap.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ServerMap::ServerMap()
{
	_serverMap = std::map<ServerKey, std::vector<Server> >();
}

ServerMap::ServerMap(const ServerMap &src)
{
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
	for (std::map<ServerKey, std::vector<Server> >::iterator it = _serverMap.begin(); it != _serverMap.end(); ++it)
	{
		close(it->first.getFd());
	}
	_serverMap.clear();
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ServerMap &ServerMap::operator=(ServerMap const &rhs)
{
	if (this != &rhs)
		_serverMap = rhs._serverMap;
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
				servers.push_back(Server(*serverName, host_port->first, host_port->second, *it));
				// Simultanously populate the server map with unique host:port keys
				// insert will fail if the key already exists ensuring that the key is unique
				_serverMap.insert(std::make_pair(ServerKey(0, host_port->first, host_port->second), std::vector<Server>()));
			}
		}
	}
	return servers;
}

// This methods populates the unbinded server keys map with the potential server keys
void ServerMap::_spawnServerKeys(std::vector<Server> &servers)
{
	// Check if the number of server keys is greater than the number of available file descriptors
	if (_serverMap.size() >= FD_SETSIZE)
	{
		std::stringstream ss;
		ss << "ServerMap: Attempting to bind more server keys than available file descriptors, FD_SETSIZE: " << FD_SETSIZE;
		Logger::log(Logger::INFO, ss.str());
		throw std::runtime_error(ss.str());
	}

	try
	{
		// Attempt to bind the server keys
		for (std::map<ServerKey, std::vector<Server> >::iterator it = _serverMap.begin(); it != _serverMap.end(); ++it)
		{
			const int fd = socket(AF_INET, SOCK_STREAM, 0);
			if (fd == -1)
			{
				std::stringstream ss;
				ss << "ServerMap: Failed to create socket for " << it->first.getHost() << ":" << it->first.getPort() << " errno: " << strerror(errno);
				Logger::log(Logger::ERROR, ss.str());
				continue;
			}

			// Set socket options
			int opt = 1;
			if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
			{
				std::stringstream ss;
				ss << "ServerMap: Failed to set SO_REUSEADDR for " << it->first.getHost() << ":" << it->first.getPort();
				Logger::log(Logger::WARNING, ss.str());
				throw std::runtime_error(ss.str());
			}

			// Make socket non-blocking
			if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
			{
				std::stringstream ss;
				ss << "ServerMap: Failed to make socket non-blocking for " << it->first.getHost() << ":" << it->first.getPort();
				Logger::log(Logger::ERROR, ss.str());
				throw std::runtime_error(ss.str());
			}

			// Setup sockaddr_in structure
			struct sockaddr_in server_addr;
			std::memset(&server_addr, 0, sizeof(server_addr));
			server_addr.sin_family = AF_INET;
			server_addr.sin_port = htons(it->first.getPort());

			// If the host is 0.0.0.0 or empty, bind to INADDR_ANY effectively binding to all interfaces
			if (it->first.getHost() == "0.0.0.0" || it->first.getHost().empty())
				server_addr.sin_addr.s_addr = INADDR_ANY;

			// Bind socket
			if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
			{
				std::stringstream ss;
				ss << "ServerMap: Failed to bind " << it->first.getHost() << ":" << it->first.getPort() << " - " << strerror(errno);
				Logger::log(Logger::ERROR, ss.str());
				throw std::runtime_error(ss.str());
			}

			// Listen on socket
			if (listen(fd, 1024) == -1)
			{
				std::stringstream ss;
				ss << "ServerMap: Failed to listen on " << it->first.getHost() << ":" << it->first.getPort() << " - " << strerror(errno);
				Logger::log(Logger::ERROR, ss.str());
				throw std::runtime_error(ss.str());
			}

			// Success replace the server key with a new one that includes the fd
			ServerKey new_key = ServerKey(fd, it->first.getHost(), it->first.getPort());
			_serverMap.erase(it->first);
			_serverMap.insert(std::make_pair(new_key, it->second));
		}
	}
	catch (std::runtime_error &e)
	{
		Logger::log(Logger::ERROR, e.what());
		throw std::runtime_error(e.what());
	}
}

void ServerMap::_populateServerMap(std::vector<Server> &servers)
{
	// Iterate through each server
	for (std::vector<Server>::iterator server_object_it = servers.begin(); server_object_it != servers.end(); ++server_object_it)
	{
		// Attempt to match the server object o a key in the server map
		for (std::map<ServerKey, std::vector<Server> >::iterator it = _serverMap.begin(); it != _serverMap.end(); ++it)
		{
			// If the server object matches the key
			if (it->first.getHost() == server_object_it->getHost() && it->first.getPort() == server_object_it->getPort())
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

const std::vector<Server> &ServerMap::getServers(ServerKey &key) const
{
	return _serverMap.at(key);
}

const Server &ServerMap::getServer(ServerKey &key, std::string &serverName) const
{
	std::vector<Server> servers = getServers(key);
	for (std::vector<Server>::const_iterator server = servers.begin(); server != servers.end(); ++server)
	{
		if (server->getServerName() == serverName)
			return *server;
	}
	throw std::out_of_range("ServerMap: Server not found");
}

bool ServerMap::hasFd(int &fd) const
{
	for (std::map<ServerKey, std::vector<Server> >::const_iterator it = _serverMap.begin(); it != _serverMap.end(); ++it)
	{
		if (it->first.getFd() == fd)
			return true;
	}
	return false;
}

/* ************************************************************************** */
