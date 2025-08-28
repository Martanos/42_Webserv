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
	FileDescriptor _socket;
	SocketAddress _address;

public:
	// Constructor
	ListeningSocket(const std::string &host, unsigned short port);

	// Non-copyable
	ListeningSocket(const ListeningSocket &);
	ListeningSocket &operator=(const ListeningSocket &);

	// Accept connection
	FileDescriptor accept();

	int getFd() const;
	const SocketAddress &getAddress() const;
};

std::ostream &operator<<(std::ostream &o, ListeningSocket const &i);

#endif /* ************************************************* LISTENINGSOCKET_H */
