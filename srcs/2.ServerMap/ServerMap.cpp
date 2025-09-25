#include "../../includes/ServerMap.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ServerMap::ServerMap()
{
	_serverMap = std::map<ListeningSocket, std::vector<Server> >();
}

ServerMap::ServerMap(const ServerMap &src)
{
	*this = src;
}

ServerMap::ServerMap(std::vector<ServerConfig> &serverConfigs)
{
	std::vector<Server> servers = _spawnServers(serverConfigs);
	_populateServerMap(servers);
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ServerMap::~ServerMap()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ServerMap &ServerMap::operator=(ServerMap const &rhs)
{
	if (this != &rhs)
	{
		_serverMap = rhs._serverMap;
	}
	return *this;
}

/*
** --------------------------------- UTILITY METHODS
*----------------------------------
*/

// Responsible for spawning the servers from the server configs server class
// will take care of spawning the locations
std::vector<Server> ServerMap::_spawnServers(std::vector<ServerConfig> &serverConfigs)
{
	std::vector<Server> servers;
	// Small optimisation for memory space
	servers.reserve(serverConfigs.size() * 2);
	// Iterate through each server config
	for (std::vector<ServerConfig>::iterator it = serverConfigs.begin(); it != serverConfigs.end(); ++it)
	{
		// Iterate through each server name
		for (std::vector<std::string>::const_iterator serverName = it->getServerNames().begin();
			 serverName != it->getServerNames().end(); ++serverName)
		{
			// Iterate through each host + port pair
			for (std::vector<std::pair<std::string, unsigned short> >::const_iterator host_port =
					 it->getHosts_ports().begin();
				 host_port != it->getHosts_ports().end(); ++host_port)
			{
				// Spawn a server
				servers.push_back(Server(*serverName, host_port->first, host_port->second, *it));
				ListeningSocket listeningSocket(host_port->first, host_port->second);
				if (listeningSocket.getFd() != -1)
				{
					_serverMap.insert(std::make_pair(listeningSocket, std::vector<Server>()));
				}
			}
		}
	}
	return servers;
}

void ServerMap::_populateServerMap(std::vector<Server> &servers)
{
	// Iterate through each server
	for (std::vector<Server>::iterator server_object_it = servers.begin(); server_object_it != servers.end();
		 ++server_object_it)
	{
		// Attempt to match the server object o a key in the server map
		for (std::map<ListeningSocket, std::vector<Server> >::iterator it = _serverMap.begin(); it != _serverMap.end();
			 ++it)
		{
			// If the server object matches the key
			if (it->first.getAddress().getHost() == server_object_it->getHost() &&
				it->first.getAddress().getPort() == server_object_it->getPort())
			{
				// Check that the vector does not contain a similar server
				// object
				for (std::vector<Server>::iterator server = it->second.begin(); server != it->second.end(); ++server)
				{
					// If it does then ignore the current server object and
					// continue to the next server object
					if (server->getServerName() == server_object_it->getServerName())
					{
						std::stringstream ss;
						ss << "ServerMap: Server " << server_object_it->getServerName()
						   << " already exists in server map, ignoring this "
							  "server object: "
						   << *server_object_it;
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
** --------------------------------- ACCESSORS
*----------------------------------
*/

std::vector<Server> &ServerMap::getServers(int fd)
{
	printf("Getting servers for key: %d\n", fd);
	std::map<ListeningSocket, std::vector<Server> >::iterator it = _serverMap.find(getListeningSocket(fd));
	if (it == _serverMap.end())
		throw std::out_of_range("ServerMap: Server not found");
	return _serverMap[getListeningSocket(fd)];
}

const Server &ServerMap::getServer(ListeningSocket &key, std::string &serverName)
{
	std::vector<Server> servers = getServers(key.getFd());
	for (std::vector<Server>::const_iterator server = servers.begin(); server != servers.end(); ++server)
	{
		if (server->getServerName() == serverName)
			return *server;
	}
	throw std::out_of_range("ServerMap: Server not found");
}

bool ServerMap::hasFd(int &fd) const
{
	for (std::map<ListeningSocket, std::vector<Server> >::const_iterator it = _serverMap.begin();
		 it != _serverMap.end(); ++it)
	{
		if (it->first.getFd() == fd)
			return true;
	}
	return false;
}

const ListeningSocket &ServerMap::getListeningSocket(int &fd)
{
	for (std::map<ListeningSocket, std::vector<Server> >::const_iterator it = _serverMap.begin();
		 it != _serverMap.end(); ++it)
	{
		if (it->first.getFd() == fd)
			return it->first;
	}
	throw std::out_of_range("ServerMap: Listening socket not found");
}

const std::map<ListeningSocket, std::vector<Server> > &ServerMap::getServerMap() const
{
	return _serverMap;
}

/* ************************************************************************** */
