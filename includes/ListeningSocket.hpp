#ifndef LISTENINGSOCKET_HPP
#define LISTENINGSOCKET_HPP

#include <iostream>
#include <string>
#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/socket.h>
#include "FileDescriptor.hpp"
#include "SocketAddress.hpp"
#include "Logger.hpp"

class ListeningSocket
{
private:
	ListeningSocket();
	ListeningSocket(const ListeningSocket &src);
	ListeningSocket &operator=(const ListeningSocket &rhs);

	FileDescriptor _socket;
	SocketAddress _address;

public:
	// Constructor
	ListeningSocket(const std::string &host, const unsigned short port);
	~ListeningSocket();

	// Accept connection
	FileDescriptor accept() const;

	// Comparator overloads
	bool operator<(const ListeningSocket &rhs) const;
	bool operator>(const ListeningSocket &rhs) const;
	bool operator<=(const ListeningSocket &rhs) const;
	bool operator>=(const ListeningSocket &rhs) const;
	bool operator==(const ListeningSocket &rhs) const;
	bool operator!=(const ListeningSocket &rhs) const;

	int getFd() const;
	const SocketAddress &getAddress() const;
};

std::ostream &operator<<(std::ostream &o, ListeningSocket const &i);

#endif /* ************************************************* LISTENINGSOCKET_H */
