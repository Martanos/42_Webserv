#ifndef SERVERMAP_HPP
#define SERVERMAP_HPP

#include "../../includes/Core/Server.hpp"
#include "../../includes/Wrapper/ListeningSocket.hpp"
#include "../../includes/Logger.hpp"
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
	std::vector<Server> _servers;
	std::vector<ListeningSocket> _listeningSockets;

	// Server map (key: listening socket, value: vector of references to servers)
	std::map<int, std::vector<Server> > _serverMap;

	// Utility Methods

	// Non-copyableServerMap(const ServerMap &src);
	ServerMap(const ServerMap &src);
	ServerMap &operator=(const ServerMap &rhs);

public:
	explicit ServerMap(const std::vector<ServerConfig> &serverConfigs);
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
