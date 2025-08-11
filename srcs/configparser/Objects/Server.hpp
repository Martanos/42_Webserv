#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>

// Main server object contains all information for server
class Server
{
private:
public:
	Server();
	Server(Server const &src);
	~Server();

	Server &operator=(Server const &rhs);
};

std::ostream &operator<<(std::ostream &o, Server const &i);

#endif /* ********************************************************** SERVER_H */
