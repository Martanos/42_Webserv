#ifndef SERVERMAP_HPP
#define SERVERMAP_HPP

#include "ListeningSocket.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "ServerConfig.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

// Object to hold the server's configuration map
class ServerMap
{
private:
	// Vector of servers themselves
	std::vector<Server> _servers;

	// Server map (key: listening socket, value: vector of references to servers)
	std::map<ListeningSocket, std::vector<Server> > _serverMap;

	// Utility Methods

public:
	ServerMap();
	ServerMap(std::vector<ServerConfig> &serverConfigs);
	ServerMap(const ServerMap &src);
	ServerMap &operator=(ServerMap const &rhs);
	~ServerMap();

	// Getters
	const std::map<ListeningSocket, std::vector<Server> > &getServerMap() const;

	// Server vectors
	std::vector<Server> &getServers(const ListeningSocket &key);

	// Mutators
	const void insertServer(const Server &server);

	// Listening sockets
	const ListeningSocket &getListeningSocket(int &fd) const;

	// Utility Methods
	bool hasFd(int &fd) const;
	void printServerMap() const;
};

#endif /* *************************************************** SERVERMANAGER_H                                          \
		*/
