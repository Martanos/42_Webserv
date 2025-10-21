#ifndef LISTENINGSOCKET_HPP
#define LISTENINGSOCKET_HPP

#include "FileDescriptor.hpp"
#include "Socket.hpp"
#include <cerrno>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

// Acts as a key for the server map
class ListeningSocket
{
private:
	Socket _socket;
	FileDescriptor _fd;

public:
	// Constructor
	ListeningSocket();
	ListeningSocket(const ListeningSocket &src);
	ListeningSocket(const std::string &host, const unsigned short port);
	~ListeningSocket();

	ListeningSocket &operator=(const ListeningSocket &rhs);

	// Accept connection
	void accept(Socket &address, FileDescriptor &clientFd) const;

	// Comparator overloads for mapping
	bool operator<(const ListeningSocket &rhs) const;
	bool operator>(const ListeningSocket &rhs) const;
	bool operator<=(const ListeningSocket &rhs) const;
	bool operator>=(const ListeningSocket &rhs) const;
	bool operator==(const ListeningSocket &rhs) const;
	bool operator!=(const ListeningSocket &rhs) const;

	FileDescriptor &getFd();
	const FileDescriptor &getFd() const;
	Socket &getAddress();
	const Socket &getAddress() const;
};

std::ostream &operator<<(std::ostream &o, ListeningSocket const &i);

#endif /* ************************************************* LISTENINGSOCKET_H                                          \
		*/
