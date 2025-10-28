#include "../../includes/Wrapper/ListeningSocket.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

ListeningSocket::ListeningSocket()
{
	_socketAddress = SocketAddress();
	_bindFd = FileDescriptor();
}

ListeningSocket::ListeningSocket(const SocketAddress &socketAddress) : _socketAddress(socketAddress)
{
	// Create socket
	_bindFd = FileDescriptor::createSocket(AF_INET, SOCK_STREAM, 0);
	if (!_bindFd.isValid())
	{
		Logger::error("ListeningSocket: Failed to create socket");
		return;
	}

	// Set socket options
	_bindFd.setReuseAddr();

	// Bind socket
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_socketAddress.getPort());
	if (inet_pton(AF_INET, _socketAddress.getHost().c_str(), &addr.sin_addr) <= 0)
	{
		Logger::error("ListeningSocket: Invalid address: " + _socketAddress.getHost());
		_bindFd = FileDescriptor();
		return;
	}

	if (bind(_bindFd.getFd(), (struct sockaddr*)&addr, sizeof(addr)) == -1)
	{
		Logger::error("ListeningSocket: Failed to bind socket to " + _socketAddress.getHost() + ":" + 
					  StrUtils::toString(_socketAddress.getPort()));
		_bindFd = FileDescriptor();
		return;
	}

	// Start listening
	if (listen(_bindFd.getFd(), 128) == -1)
	{
		Logger::error("ListeningSocket: Failed to listen on socket");
		_bindFd = FileDescriptor();
		return;
	}

	Logger::debug("ListeningSocket: Created listening socket on " + _socketAddress.getHost() + ":" + 
				  StrUtils::toString(_socketAddress.getPort()));
	Logger::debug("ListeningSocket: Socket fd: " + StrUtils::toString(_bindFd.getFd()) + 
				  ", family: " + StrUtils::toString(AF_INET) + 
				  ", type: " + StrUtils::toString(SOCK_STREAM));
}

ListeningSocket::ListeningSocket(const ListeningSocket &src) : _socketAddress(src._socketAddress), _bindFd(src._bindFd)
{
}

ListeningSocket &ListeningSocket::operator=(const ListeningSocket &rhs)
{
	if (this != &rhs)
	{
		_socketAddress = rhs._socketAddress;
		_bindFd = rhs._bindFd;
	}
	return *this;
}

ListeningSocket::~ListeningSocket()
{
}

void ListeningSocket::accept(SocketAddress &address, FileDescriptor &clientFd) const
{
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	
	int clientFdInt = ::accept(_bindFd.getFd(), (struct sockaddr*)&clientAddr, &clientAddrLen);
	if (clientFdInt == -1)
	{
		Logger::error("ListeningSocket: Failed to accept connection");
		clientFd = FileDescriptor();
		return;
	}

	clientFd = FileDescriptor::createFromDup(clientFdInt);
	
	// Set up client address
	char clientHost[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientAddr.sin_addr, clientHost, INET_ADDRSTRLEN);
	address = SocketAddress(std::string(clientHost), ntohs(clientAddr.sin_port));
}

bool ListeningSocket::operator<(const ListeningSocket &rhs) const
{
	return _socketAddress < rhs._socketAddress;
}

bool ListeningSocket::operator>(const ListeningSocket &rhs) const
{
	return _socketAddress > rhs._socketAddress;
}

bool ListeningSocket::operator<=(const ListeningSocket &rhs) const
{
	return _socketAddress <= rhs._socketAddress;
}

bool ListeningSocket::operator>=(const ListeningSocket &rhs) const
{
	return _socketAddress >= rhs._socketAddress;
}

bool ListeningSocket::operator==(const ListeningSocket &rhs) const
{
	return _socketAddress == rhs._socketAddress;
}

bool ListeningSocket::operator!=(const ListeningSocket &rhs) const
{
	return _socketAddress != rhs._socketAddress;
}

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

std::ostream &operator<<(std::ostream &o, ListeningSocket const &i)
{
	o << "ListeningSocket(" << i.getAddress() << ")";
	return o;
}