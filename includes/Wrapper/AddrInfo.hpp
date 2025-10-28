#ifndef ADDRINFO_HPP
#define ADDRINFO_HPP

#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>

#include "../../includes/Global/Logger.hpp"

class AddrInfo
{
private:
	struct addrinfo *_result;

public:
	// Constructor
	AddrInfo();
	AddrInfo(const std::string &host, const std::string &port, int family = AF_UNSPEC);

	// Destructor
	~AddrInfo();

	// Non-copyable
	AddrInfo(const AddrInfo &);
	AddrInfo &operator=(const AddrInfo &);

	// Access
	const struct addrinfo *get() const;
	const struct addrinfo *operator->() const;

	// Iterator-like access for multiple results
	const struct addrinfo *begin() const;
	const struct addrinfo *next(const struct addrinfo *current) const;
};

std::ostream &operator<<(std::ostream &o, AddrInfo const &i);

#endif /* ******************************************************** ADDRINFO_H                                          \
		*/
