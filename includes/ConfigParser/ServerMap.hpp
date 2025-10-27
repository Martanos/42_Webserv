#ifndef SERVERMAP_HPP
#define SERVERMAP_HPP

#include "../../includes/Core/Server.hpp"
#include "../../includes/Wrapper/ListeningSocket.hpp"
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

// Object to hold the server's configuration map
class ServerMap
{
private:
	std::vector<Server> _servers; // borrowed references to servers from ConfigTranslator

	// Server map (key: binded socket, value: vector of references to servers)
	std::map<ListeningSocket, std::vector<Server> > _serverMap;

	void _buildServerMap();

public:
	explicit ServerMap(std::vector<Server> &servers);
	ServerMap(ServerMap const &src);
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
