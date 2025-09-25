#ifndef LISTENINGSOCKET_HPP
#define LISTENINGSOCKET_HPP

#include "FileDescriptor.hpp"
#include "SocketAddress.hpp"
#include <cerrno>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

class ListeningSocket
{
private:
	ListeningSocket();

	ListeningSocket &operator=(const ListeningSocket &rhs);

	FileDescriptor _socket;
	SocketAddress _address;

public:
	// Constructor
	ListeningSocket(const ListeningSocket &src);
	ListeningSocket(const std::string &host, const unsigned short port);
	~ListeningSocket();

	// Accept connection
	FileDescriptor accept(SocketAddress &address) const;

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

#endif /* ************************************************* LISTENINGSOCKET_H                                          \
		*/
