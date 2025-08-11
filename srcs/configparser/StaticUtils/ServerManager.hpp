#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "Server.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include "Location.hpp"
#include "Logger.hpp"
#include "Socket.hpp"

class ServerManager
{
private:
	struct ServerKey
	{
		std::string host;
		unsigned short port;
		int fd;

		ServerKey(std::string host, unsigned short port) : host(host = "localhost"), port(port = 80), fd(fd = -1) {}

		~ServerKey() {}

		bool operator<(const ServerKey &rhs) const
		{
			return std::make_pair(host, port) < std::make_pair(rhs.host, rhs.port);
		}

		bool operator>(const ServerKey &rhs) const
		{
			return std::make_pair(host, port) > std::make_pair(rhs.host, rhs.port);
		}

		bool operator!=(const ServerKey &rhs) const
		{
			return !(*this == rhs);
		}

		bool operator==(const ServerKey &rhs) const
		{
			return host == rhs.host && port == rhs.port;
		}
	};

	// FD map to associated servers
	std::map<ServerKey, std::vector<Server &> > _fdToServers;

	// Spawn server objects from server config information
	std::vector<Server> _spawnServers(std::vector<ServerConfig> &serverConfigs)
	{
		std::vector<Server> servers;

		for (std::vector<ServerConfig>::iterator serverConfig_it = serverConfigs.begin(); serverConfig_it != serverConfigs.end(); ++serverConfig_it)
		{
			for (std::vector<std::string>::iterator name_it = serverConfig_it->getServerNames().begin(); name_it != serverConfig_it->getServerNames().end(); ++name_it)
			{
				for (std::vector<std::pair<std::string, unsigned short> >::iterator host_port_it = serverConfig_it->getHosts_ports().begin(); host_port_it != serverConfig_it->getHosts_ports().end(); ++host_port_it)
				{
					Server server;
					server.setServerName(*name_it);
					server.setHost_port(*host_port_it);
					server.setRoot(serverConfig_it->getRoot());
					server.setIndexes(serverConfig_it->getIndexes());
					server.setAutoindex(serverConfig_it->getAutoindex());
					server.setClientMaxBodySize(serverConfig_it->getClientMaxBodySize());
					server.setStatusPages(serverConfig_it->getErrorPages());
					server.setLocations(serverConfig_it->getLocations()); // TODO: Proper location copying
					servers.push_back(server);
				}
			}
		}
	}

	// Sift through servers for duplicates and attach to fd map
	void _siftServers(std::vector<Server> &servers)
	{
		for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
		{
			ServerKey key(it->getHost_port().first, it->getHost_port().second);
			// First find if key exists in fdToServers
			if (_fdToServers.find(key) == _fdToServers.end())
			{
				// If not, create new vector and add server
				_fdToServers[key] = std::vector<Server &>();
				_fdToServers[key].push_back(*it);
				{
					std::stringstream ss;
					ss << "Server " << it->getServerName() << " at " << it->getHost_port().first << ":" << it->getHost_port().second << " added to fd map" << std::endl;
					Logger::log(Logger::INFO, ss.str());
				}
			}
			else
			{
				// If key exists, check if server is already in vector
				if (std::find(_fdToServers[key].begin(), _fdToServers[key].end(), *it) == _fdToServers[key].end())
				{
					// If not, add server to vector
					_fdToServers[key].push_back(*it);
					{
						std::stringstream ss;
						ss << "Server " << it->getServerName() << " at " << it->getHost_port().first << ":" << it->getHost_port().second << " added to fd map" << std::endl;
						Logger::log(Logger::INFO, ss.str());
					}
				}
				else
				{
					std::stringstream ss;
					ss << "Server " << it->getServerName() << " at " << it->getHost_port().first << ":" << it->getHost_port().second << " duplicate found ignoring" << std::endl;
					Logger::log(Logger::INFO, ss.str());
				}
			}
		}
		// Clear servers to conserve memory
		servers.clear();
	}

	void _bindServers()
	{
		for (std::map<ServerKey, std::vector<Server &> >::iterator it = _fdToServers.begin(); it != _fdToServers.end(); ++it)
		{
			Socket socket(it->first.host, it->first.port);
			socket.bind();
			socket.listen();
			it->second[0].setSocket(socket);
			it->first.fd = socket.getFd();
			{
				std::stringstream ss;
				ss << "Server " << it->second[0].getServerName() << " at " << it->first.host << ":" << it->first.port << " bound to fd " << it->first.fd << std::endl;
				Logger::log(Logger::INFO, ss.str());
			}
		}
		// TODO: Set socket to non-blocking
		// TODO: Add socket to epoll
	}

public:
	ServerManager();
	ServerManager(ServerManager const &src);
	~ServerManager();

	ServerManager &operator=(ServerManager const &rhs);
};

std::ostream &operator<<(std::ostream &o, ServerManager const &i);

#endif /* ************************************************** SERVERMANAGER_H */
