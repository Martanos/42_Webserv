#ifndef SERVERMAP_HPP
#define SERVERMAP_HPP

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include "Server.hpp"
#include "ServerConfig.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "ServerKey.hpp"

// The job of this class is to divide up the information taken from the config into manageble maps for later server operations
// 1. Divide up ServerConfigs into Server classes (This reduces navigation complexity as well as makes it clearer which class is doing what)
// 2. Spawn the map of servers where key is a pair pair of fd to host + port pair simultaneously binding to the fd and listening port to get the fd
// 3. Attempt to deep copy the Server classes to their repective value vectors if a match is found its ignored
// Accessors returns a const reference to a server object
class ServerMap
{
private:
	// Server key struct
	std::map<ServerKey, std::vector<Server> > _serverMap;

	// Main Methods
	void _spawnServerMap(std::vector<ServerConfig> &serverConfigs);

	// Utility Methods
	std::vector<Server> _spawnServers(std::vector<ServerConfig> &serverConfigs);
	void _spawnServerKeys(std::vector<Server> &servers);
	void _populateServerMap(std::vector<Server> &servers);

public:
	ServerMap();
	ServerMap(ServerMap const &src);
	ServerMap(std::vector<ServerConfig> &serverConfigs);
	ServerMap &operator=(ServerMap const &rhs);
	~ServerMap();

	// Getters
	const std::map<ServerKey, std::vector<Server> > &getServerMap() const;

	// Server vectors
	const std::vector<Server> &getServers(ServerKey &key) const;

	// Individual servers
	const Server &getServer(ServerKey &key, std::string &serverName) const;

	// Utility Methods
	bool hasFd(int &fd) const;
};

#endif /* *************************************************** SERVERMANAGER_H */
