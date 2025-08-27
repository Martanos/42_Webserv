#ifndef SERVERKEY_HPP
#define SERVERKEY_HPP

#include <iostream>
#include <string>

class ServerKey
{
private:
	int _fd;
	std::string _host;
	unsigned short _port;

public:
	ServerKey();
	ServerKey(ServerKey const &src);
	ServerKey(int fd, std::string host, unsigned short port);
	~ServerKey();
	ServerKey &operator=(ServerKey const &rhs);

	// Operator overloads
	bool operator<(const ServerKey &rhs);
	bool operator>(const ServerKey &rhs);

	bool operator<=(const ServerKey &rhs);
	bool operator>=(const ServerKey &rhs);

	bool operator==(const ServerKey &rhs);
	bool operator!=(const ServerKey &rhs);

	// Getters
	int getFd();
	const int getFd() const;
	std::string getHost();
	const std::string getHost() const;
	unsigned short getPort();
	const unsigned short getPort() const;

	// Setters
	void setFd(int fd);
	void setHost(std::string host);
	void setPort(unsigned short port);
};

std::ostream &operator<<(std::ostream &o, ServerKey const &i);

#endif /* ******************************************************* SERVERKEY_H */
