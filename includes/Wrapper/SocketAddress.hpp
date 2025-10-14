#ifndef SOCKETADDRESS_HPP
#define SOCKETADDRESS_HPP

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

// Class wrapper for socket addresses using sockaddr_storage for compatibility
// TODO: think about just making this a socket class
class SocketAddress
{
private:
	struct sockaddr_storage _storage; // Universal storage for all address types
	socklen_t _addrLen;				  // Actual length of stored address
	int _family;					  // Cached family for quick access
	std::string _host;				  // Original host string for compatibility
	unsigned short _port;			  // Cached port for quick access

	// Private helpers
	void _updateCachedValues();
	const struct sockaddr *_getSockAddr() const;
	struct sockaddr *_getSockAddr();
	const struct sockaddr_in *_getSockAddrIn() const;
	struct sockaddr_in *_getSockAddrIn();
	const struct sockaddr_in6 *_getSockAddrIn6() const;
	struct sockaddr_in6 *_getSockAddrIn6();

public:
	// Orthodox Canonical Class Form
	SocketAddress();
	SocketAddress(const int port, const std::string ip); // Keep old signature
	SocketAddress(const std::string &host, unsigned short port);
	SocketAddress(const SocketAddress &src);
	~SocketAddress();

	// Operators
	SocketAddress &operator=(const SocketAddress &rhs);
	bool operator==(const SocketAddress &rhs) const;
	bool operator!=(const SocketAddress &rhs) const;
	bool operator<(const SocketAddress &rhs) const;
	bool operator>(const SocketAddress &rhs) const;
	bool operator<=(const SocketAddress &rhs) const;
	bool operator>=(const SocketAddress &rhs) const;

	// Getters
	struct sockaddr_storage *getSockAddr();
	int getFamily() const;
	socklen_t &getSize();
	unsigned short getPort() const;
	const struct sockaddr_in &getIPV4() const;	// Keep old name
	const struct sockaddr_in6 &getIPV6() const; // Keep old name
	const struct sockaddr_in &get() const;		// Keep old method
	std::string getHost() const;
	socklen_t getAddrLen() const;
	std::string getHostString() const;
	std::string getPortString() const;

	// Setters - DEPRECATED but kept for compatibility
	void setFamily(int family);
	void setPort(int port);
	void setHost(const std::string &host);
	void setAddrLen(socklen_t addrLen);

	// Validators
	bool isIPv4() const;
	bool isIPv6() const;
	bool isValid() const;
	bool isEmpty() const;

	// Static factory methods - KEEP EXISTING SIGNATURES
	static SocketAddress createIPv4(const std::string &host, unsigned short port);
	static SocketAddress createIPv6(const std::string &host, unsigned short port);
	static SocketAddress createAuto(const std::string &host, unsigned short port);

	// New methods for sockaddr_storage compatibility
	static SocketAddress createFromStorage(const struct sockaddr_storage &storage, socklen_t addrLen);
	void clear();
};

std::ostream &operator<<(std::ostream &o, const SocketAddress &i);

#endif /* *************************************************** SOCKETADDRESS_H */