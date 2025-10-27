#ifndef LISTENINGSOCKET_HPP
#define LISTENINGSOCKET_HPP

#include "FileDescriptor.hpp"
#include "SocketAddress.hpp"
#include <iostream>

// Wrapper for listening socketss
class ListeningSocket
{
private:
<<<<<<< HEAD:includes/ListeningSocket.hpp
	FileDescriptor _socket;
	SocketAddress _address;
=======
	SocketAddress _socketAddress; // Socket address info
	FileDescriptor _bindFd;		  // Fd when binded

	// Non-copyable
>>>>>>> ConfigParserRefactor:includes/Wrapper/ListeningSocket.hpp

public:
	explicit ListeningSocket(const SocketAddress &socketAddress);
	ListeningSocket(const ListeningSocket &src);
	ListeningSocket &operator=(const ListeningSocket &rhs);
	~ListeningSocket();
	ListeningSocket();

	ListeningSocket &operator=(const ListeningSocket &rhs);
	// Accept connection
	void accept(SocketAddress &address, FileDescriptor &clientFd) const;

	// Comparator overloads for mapping
	bool operator<(const ListeningSocket &rhs) const;
	bool operator>(const ListeningSocket &rhs) const;
	bool operator<=(const ListeningSocket &rhs) const;
	bool operator>=(const ListeningSocket &rhs) const;
	bool operator==(const ListeningSocket &rhs) const;
	bool operator!=(const ListeningSocket &rhs) const;

	FileDescriptor &getFd();
	const FileDescriptor &getFd() const;
	SocketAddress &getAddress();
	const SocketAddress &getAddress() const;
};

std::ostream &operator<<(std::ostream &o, ListeningSocket const &i);

#endif /* ************************************************* LISTENINGSOCKET_H                                          \
		*/
