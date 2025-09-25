#ifndef SOCKETADDRESS_HPP
#define SOCKETADDRESS_HPP

#include "AddrInfo.hpp"
#include "IPAddressParser.hpp"
#include "Logger.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

// Class wrapper for socket addresses c structs
class SocketAddress
{
private:
	union _addr
	{
		struct sockaddr_in _addr4;
		struct sockaddr_in6 _addr6;
		struct sockaddr _addr_generic;
	} _addr;

	int _family;
	int _port;
	std::string _host;
	socklen_t _addrLen;

public:
	SocketAddress();
	SocketAddress(const int port, const std::string ip);
	SocketAddress(const std::string &host, unsigned short port);
	SocketAddress(SocketAddress const &src);
	~SocketAddress();

	// Operators
	SocketAddress &operator=(SocketAddress const &rhs);
	bool operator==(SocketAddress const &rhs) const;
	bool operator!=(SocketAddress const &rhs) const;

	// Getters
	const struct sockaddr *getSockAddr() const;
	int getFamily() const;
	unsigned short getPort() const;
	const struct sockaddr_in &getIPV4() const;
	const struct sockaddr_in6 &getIPV6() const;
	const struct sockaddr_in &get() const;
	std::string getHost() const;
	socklen_t getAddrLen() const;
	std::string getHostString() const;
	std::string getPortString() const;
	socklen_t getSize() const;

	// Setters
	void setFamily(int family);
	void setPort(int port);
	void setHost(const std::string &host);
	void setAddrLen(socklen_t addrLen);

	// Validators
	bool isIPv4() const;
	bool isIPv6() const;

	// Methods
	static SocketAddress createIPv4(const std::string &host, unsigned short port)
	{
		SocketAddress addr;
		addr._family = AF_INET;
		addr._addrLen = sizeof(struct sockaddr_in);
		addr._addr._addr4.sin_family = AF_INET;
		addr._addr._addr4.sin_port = htons(port);

		if (host == "0.0.0.0" || host.empty())
		{
			addr._addr._addr4.sin_addr.s_addr = INADDR_ANY;
		}
		else
		{
			uint32_t ipAddr;
			if (!IPAddressParser::parseIPv4(host, ipAddr))
			{
				std::stringstream ss;
				ss << "Invalid IPv4 address: " << host;
				Logger::log(Logger::ERROR, ss.str());
				throw std::runtime_error(ss.str());
			}
			addr._addr._addr4.sin_addr.s_addr = ipAddr;
		}
		return addr;
	}

	static SocketAddress createIPv6(const std::string &host, unsigned short port)
	{
		SocketAddress addr;
		addr._family = AF_INET6;
		addr._addrLen = sizeof(struct sockaddr_in6);
		addr._addr._addr6.sin6_family = AF_INET6;
		addr._addr._addr6.sin6_port = htons(port);

		if (host == "::" || host.empty())
		{
			addr._addr._addr6.sin6_addr = in6addr_any;
		}
		else
		{
			if (!IPAddressParser::parseIPv6(host, addr._addr._addr6.sin6_addr))
			{
				throw std::runtime_error("Invalid IPv6 address: " + host);
			}
		}
		return addr;
	}

	static SocketAddress createAuto(const std::string &host, unsigned short port)
	{
		if (IPAddressParser::looksLikeIPv4(host))
		{
			return createIPv4(host, port);
		}
		else if (IPAddressParser::looksLikeIPv6(host))
		{
			return createIPv6(host, port);
		}
		else
		{
			// For hostname resolution, still need getaddrinfo (it's in allowed
			// functions) This is the only way to resolve hostnames without
			// banned functions
			AddrInfo addrInfo(host, "80"); // Use port 80 as dummy
			const struct addrinfo *info = addrInfo.get();

			if (info->ai_family == AF_INET)
			{
				SocketAddress addr;
				addr._family = AF_INET;
				addr._addrLen = sizeof(struct sockaddr_in);
				addr._addr._addr4 = *reinterpret_cast<const struct sockaddr_in *>(info->ai_addr);
				addr._addr._addr4.sin_port = htons(port); // Override with our port
				return addr;
			}
			else if (info->ai_family == AF_INET6)
			{
				SocketAddress addr;
				addr._family = AF_INET6;
				addr._addrLen = sizeof(struct sockaddr_in6);
				addr._addr._addr6 = *reinterpret_cast<const struct sockaddr_in6 *>(info->ai_addr);
				addr._addr._addr6.sin6_port = htons(port); // Override with our port
				return addr;
			}
			else
			{
				throw std::runtime_error("Unsupported address family for host: " + host);
			}
		}
	}
};

std::ostream &operator<<(std::ostream &o, SocketAddress const &i);

#endif /* *************************************************** SOCKETADDRESS_H                                          \
		*/
