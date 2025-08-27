#include "ServerKey.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ServerKey::ServerKey()
{
	this->_fd = 0;
	this->_host = "";
	this->_port = 0;
}

ServerKey::ServerKey(const ServerKey &src)
{
	if (this != &src)
		*this = src;
}

ServerKey::ServerKey(int fd, std::string host, unsigned short port)
{
	this->_fd = fd;
	this->_host = host;
	this->_port = port;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ServerKey::~ServerKey()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ServerKey &ServerKey::operator=(ServerKey const &rhs)
{
	if (this != &rhs)
	{
		this->_fd = rhs._fd;
		this->_host = rhs._host;
		this->_port = rhs._port;
	}
	return *this;
}

bool ServerKey::operator<(const ServerKey &rhs)
{
	if (this->_host != rhs._host && this->_port != rhs._port)
		return (this->_host < rhs._host && this->_port < rhs._port);
	else
		return (this->_fd < rhs._fd);
}

bool ServerKey::operator>(const ServerKey &rhs)
{
	return *this < rhs;
}

bool ServerKey::operator<=(const ServerKey &rhs)
{
	return *this < rhs || *this == rhs;
}

bool ServerKey::operator>=(const ServerKey &rhs)
{
	return *this > rhs || *this == rhs;
}

bool ServerKey::operator==(const ServerKey &rhs)
{
	if (this->_host == rhs._host && this->_port == rhs._port)
		return true;
	else
		return this->_fd == rhs._fd;
	return false;
}

bool ServerKey::operator!=(const ServerKey &rhs)
{
	return !(*this == rhs);
}

std::ostream &operator<<(std::ostream &o, ServerKey const &i)
{
	o << "fd: " << i.getFd() << " host: " << i.getHost() << " port: " << i.getPort();
	return o;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

int ServerKey::getFd()
{
	return this->_fd;
}

const int ServerKey::getFd() const
{
	return this->_fd;
}

const std::string ServerKey::getHost() const
{
	return this->_host;
}

std::string ServerKey::getHost()
{
	return this->_host;
}

const unsigned short ServerKey::getPort() const
{
	return this->_port;
}

unsigned short ServerKey::getPort()
{
	return this->_port;
}

void ServerKey::setFd(int fd)
{
	this->_fd = fd;
}

void ServerKey::setHost(std::string host)
{
	this->_host = host;
}

void ServerKey::setPort(unsigned short port)
{
	this->_port = port;
}

/* ************************************************************************** */
