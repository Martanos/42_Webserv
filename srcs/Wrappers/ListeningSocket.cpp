#include "../../includes/Wrapper/ListeningSocket.hpp"
#include "../../includes/Global/Logger.hpp"
#include <cstring>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ListeningSocket::ListeningSocket(const ListeningSocket &src)
{
	*this = src;
}

ListeningSocket::ListeningSocket(const SocketAddress &socketAddress) : _socketAddress(socketAddress)
{
	errno = 0;
	_bindFd =
		FileDescriptor::createSocket(socketAddress.getFamily(), socketAddress.getFamily(), socketAddress.getPort());
	if (!_bindFd.isValid())
		throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
	_bindFd.setReuseAddr();
	if (::bind(_bindFd.getFd(), reinterpret_cast<const struct sockaddr *>(socketAddress.getSockAddr()),
			   socketAddress.getSize()) == -1)
		throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
	if (::listen(_bindFd.getFd(), SOMAXCONN) == -1)
		throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ListeningSocket::~ListeningSocket()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ListeningSocket &ListeningSocket::operator=(ListeningSocket const &rhs)
{
	if (this != &rhs)
	{
		_socketAddress = rhs._socketAddress;
		_bindFd = rhs._bindFd;
	}
	return *this;
}

std::ostream &operator<<(std::ostream &o, ListeningSocket const &i)
{
	o << "ListeningSocket: " << i.getFd() << " " << i.getAddress();
	return o;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void ListeningSocket::accept(SocketAddress &remoteAddr, FileDescriptor &clientFd) const
{
	errno = 0;
	socklen_t addrLen = remoteAddr.getSize();
	clientFd = FileDescriptor::createFromAccept(
		_bindFd.getFd(), reinterpret_cast<struct sockaddr *>(remoteAddr.getSockAddr()), &addrLen);
	if (!clientFd.isValid())
		throw std::runtime_error("Failed to accept connection: " + std::string(strerror(errno)));
	// Update the remoteAddr with the actual address length returned by accept
	remoteAddr.setAddrLen(addrLen);
}

/*
** --------------------------------- COMPARATOR
*---------------------------------
*/

bool ListeningSocket::operator<(const ListeningSocket &rhs) const
{
	return _bindFd.getFd() < rhs._bindFd.getFd();
}

bool ListeningSocket::operator>(const ListeningSocket &rhs) const
{
	return _bindFd.getFd() > rhs._bindFd.getFd();
}

bool ListeningSocket::operator<=(const ListeningSocket &rhs) const
{
	return _bindFd.getFd() <= rhs._bindFd.getFd();
}

bool ListeningSocket::operator>=(const ListeningSocket &rhs) const
{
	return _bindFd.getFd() >= rhs._bindFd.getFd();
}

bool ListeningSocket::operator==(const ListeningSocket &rhs) const
{
	return _bindFd.getFd() == rhs._bindFd.getFd();
}

bool ListeningSocket::operator!=(const ListeningSocket &rhs) const
{
	return _bindFd.getFd() != rhs._bindFd.getFd();
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

FileDescriptor &ListeningSocket::getFd()
{
	return _bindFd;
}

const FileDescriptor &ListeningSocket::getFd() const
{
	return _bindFd;
}

SocketAddress &ListeningSocket::getAddress()
{
	return _socketAddress;
}

const SocketAddress &ListeningSocket::getAddress() const
{
	return _socketAddress;
}

/* ************************************************************************** */
