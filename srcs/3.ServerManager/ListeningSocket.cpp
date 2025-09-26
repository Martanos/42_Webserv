#include "../../includes/ListeningSocket.hpp"
#include "../../includes/Logger.hpp"
#include <cstring>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ListeningSocket::ListeningSocket() : _socket(), _address()
{
	// Default constructor - creates invalid socket
	// Should be used with assignment operator or proper initialization
}

ListeningSocket::ListeningSocket(const ListeningSocket &src) : _socket(src._socket), _address(src._address)
{
	// Copy constructor - creates a copy of the socket
	// Note: This creates a shallow copy of the file descriptor
	// TODO: Implement proper socket duplication
}

ListeningSocket::ListeningSocket(const std::string &host, const unsigned short port) : _socket(), _address(host, port)
{

	_socket = FileDescriptor::createSocket(AF_INET, SOCK_STREAM, 0);
	if (_socket.getFd() == -1)
	{
		std::stringstream ss;
		ss << "Failed to create socket";
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	try
	{
		_socket.setReuseAddr();
		if (bind(_socket.getFd(), reinterpret_cast<struct sockaddr *>(_address.getSockAddr()), _address.getSize()) ==
			-1)
		{
			std::stringstream ss;
			ss << "Failed to bind socket: " << strerror(errno) << " on " << _address;
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}

		if (listen(_socket.getFd(), SOMAXCONN) == -1)
		{
			std::stringstream ss;
			ss << "Failed to listen on socket: " << strerror(errno);
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}
	}
	catch (...)
	{
		_socket = FileDescriptor();
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
