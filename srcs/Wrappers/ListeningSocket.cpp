#include "ListeningSocket.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ListeningSocket::ListeningSocket()
{
	throw std::runtime_error("Default constructor not implemented");
}

ListeningSocket::ListeningSocket(const std::string &host, unsigned short port)
	: _socket(socket(AF_INET, SOCK_STREAM, 0)), _address(host, port)
{

	if (!_socket.isValid())
	{
		std::stringstream ss;
		ss << "Failed to create socket";
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	try
	{
		_socket.setReuseAddr();
		_socket.setNonBlocking();

		if (bind(_socket, _address.getSockAddr(), _address.getSize()) == -1)
		{
			std::stringstream ss;
			ss << "Failed to bind socket: " << strerror(errno);
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}

		if (listen(_socket, SOMAXCONN) == -1)
		{
			std::stringstream ss;
			ss << "Failed to listen on socket: " << strerror(errno);
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}
	}
	catch (...)
	{
		throw;
	}
}

ListeningSocket::ListeningSocket(const ListeningSocket &src)
{
	(void)src;
	throw std::runtime_error("Copy constructor not implemented");
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
	(void)rhs;
	throw std::runtime_error("Assignment operator not implemented");
}

std::ostream &operator<<(std::ostream &o, ListeningSocket const &i)
{
	o << "ListeningSocket: " << i.getFd() << " " << i.getAddress();
	return o;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

FileDescriptor ListeningSocket::accept()
{
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);

	int clientFd = ::accept(_socket, reinterpret_cast<struct sockaddr *>(&clientAddr), &clientAddrLen);
	if (clientFd == -1)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
		{
			Logger::log(Logger::INFO, "No connections available");
			return FileDescriptor(-1);
		}
		std::stringstream ss;
		ss << "accept failed: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}

	return FileDescriptor(clientFd);
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

int ListeningSocket::getFd() const
{
	return _socket.getFd();
}

const SocketAddress &ListeningSocket::getAddress() const
{
	return _address;
}

/* ************************************************************************** */
