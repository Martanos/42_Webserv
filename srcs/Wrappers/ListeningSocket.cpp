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

ListeningSocket::ListeningSocket(const Socket &socket) : _socket(socket)
{
	try
	{
		_bindFd = FileDescriptor::createSocket(_socket.getFamily(), _socket.getType(), _socket.getProtocol());
		_bindFd.setReuseAddr();
		if (::bind(_bindFd.getFd(), reinterpret_cast<struct sockaddr *>(_socket.getSockAddr()), _socket.getSize()) ==
			-1)
		{
			std::stringstream ss;
			ss << "Failed to bind socket: " << strerror(errno) << " on " << _socket;
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}

		if (::listen(_bindFd.getFd(), SOMAXCONN) == -1)
		{
			std::stringstream ss;
			ss << "Failed to listen on socket: " << strerror(errno);
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}
	}
	catch (std::runtime_error &e)
	{
		std::stringstream ss;
		ss << "ListeningSocket: Failed to bind socket: " << e.what();
		Logger::warning(ss.str());
		throw std::runtime_error(ss.str());
	}
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
		_socket = rhs._socket;
		_address = rhs._address;
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
	clientFd = FileDescriptor::createFromAccept(
		_socket.getFd(), reinterpret_cast<struct sockaddr *>(remoteAddr.getSockAddr()), &remoteAddr.getSize());
	if (!clientFd.isValid())
	{
		std::stringstream ss;
		ss << "[" << __FILE__ << ":" << __LINE__ << "] accept failed: on socket " << _address.getHostString() << ":"
		   << _address.getPort() << " " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
	}
	// Update the remoteAddr with the actual address length returned by accept
	remoteAddr.setAddrLen(remoteAddr.getSize());
}

/*
** --------------------------------- COMPARATOR
*---------------------------------
*/

bool ListeningSocket::operator<(const ListeningSocket &rhs) const
{
	return _socket.getFd() < rhs._socket.getFd();
}

bool ListeningSocket::operator>(const ListeningSocket &rhs) const
{
	return _socket.getFd() > rhs._socket.getFd();
}

bool ListeningSocket::operator<=(const ListeningSocket &rhs) const
{
	return _socket.getFd() <= rhs._socket.getFd();
}

bool ListeningSocket::operator>=(const ListeningSocket &rhs) const
{
	return _socket.getFd() >= rhs._socket.getFd();
}

bool ListeningSocket::operator==(const ListeningSocket &rhs) const
{
	return _socket.getFd() == rhs._socket.getFd();
}

bool ListeningSocket::operator!=(const ListeningSocket &rhs) const
{
	return _socket.getFd() != rhs._socket.getFd();
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

FileDescriptor &ListeningSocket::getFd()
{
	return _socket;
}

const FileDescriptor &ListeningSocket::getFd() const
{
	return _socket;
}

SocketAddress &ListeningSocket::getAddress()
{
	return _address;
}

const SocketAddress &ListeningSocket::getAddress() const
{
	return _address;
}

/* ************************************************************************** */
