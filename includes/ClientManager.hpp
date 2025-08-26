#ifndef CLIENTMANAGER_HPP
#define CLIENTMANAGER_HPP

#include <iostream>
#include <string>
#include "Client.hpp"

// This class manages clients and their connections
class ClientManager
{

public:
	ClientManager();
	ClientManager(ClientManager const &src);
	~ClientManager();

	ClientManager &operator=(ClientManager const &rhs);

private:
	std::map<int, Client> _clients; // clients that are currently active

public:
	void addClient(int fd, Client &client);
	void removeClient(int fd);
	Client &getClient(int fd);
	bool hasClient(int fd);
	void handleEvent(int fd);
};

std::ostream &operator<<(std::ostream &o, ClientManager const &i);

#endif /* *************************************************** CLIENTMANAGER_H */
