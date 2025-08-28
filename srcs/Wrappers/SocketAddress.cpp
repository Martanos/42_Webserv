#include "SocketAddress.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

SocketAddress::SocketAddress() : _family(AF_INET), _port(0), _host(""), _addrLen(sizeof(struct sockaddr_in))
{
	std::memset(&_addr, 0, sizeof(_addr));
	_addr._addr4.sin_family = AF_INET;
}

// Enhanced constructor using getaddrinfo for validation
SocketAddress::SocketAddress(const std::string &host, unsigned short port) : _family(AF_INET)
{
	std::memset(&_addr, 0, sizeof(_addr));

	// Handle special cases with custom logic (faster)
	if (host == "0.0.0.0" || host.empty())
	{
		_family = AF_INET;
		_addrLen = sizeof(struct sockaddr_in);
		_addr._addr4.sin_family = AF_INET;
		_addr._addr4.sin_port = htons(port);
		_addr._addr4.sin_addr.s_addr = INADDR_ANY;
		return;
	}

	if (host == "::")
	{
		_family = AF_INET6;
		_addrLen = sizeof(struct sockaddr_in6);
		_addr._addr6.sin6_family = AF_INET6;
		_addr._addr6.sin6_port = htons(port);
		_addr._addr6.sin6_addr = in6addr_any;
		return;
	}

	// For everything else, use getaddrinfo (comprehensive validation)
	struct addrinfo hints, *result;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // Allow both IPv4 and IPv6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV; // Only for IP addresses, not hostnames

	// Convert port to string for getaddrinfo
	std::ostringstream portStr;
	portStr << port;

	int status = getaddrinfo(host.c_str(), portStr.str().c_str(), &hints, &result);
	if (status != 0)
	{
		// If numeric parsing failed, try hostname resolution
		hints.ai_flags = 0; // Allow hostname resolution
		status = getaddrinfo(host.c_str(), portStr.str().c_str(), &hints, &result);

		if (status != 0)
		{
			std::stringstream ss;
			ss << "getaddrinfo failed: " << gai_strerror(status);
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}
	}

	try
	{
		// Use the first result (prefer IPv4 for compatibility)
		struct addrinfo *preferred = result;
		for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next)
		{
			if (rp->ai_family == AF_INET)
			{
				preferred = rp;
				break; // Prefer IPv4
			}
		}

		if (preferred->ai_family == AF_INET)
		{
			_family = AF_INET;
			_addrLen = sizeof(struct sockaddr_in);
			_addr._addr4 = *reinterpret_cast<struct sockaddr_in *>(preferred->ai_addr);
		}
		else if (preferred->ai_family == AF_INET6)
		{
			_family = AF_INET6;
			_addrLen = sizeof(struct sockaddr_in6);
			_addr._addr6 = *reinterpret_cast<struct sockaddr_in6 *>(preferred->ai_addr);
		}
		else
		{
			std::stringstream ss;
			ss << "Unsupported address family: " << preferred->ai_family;
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}
		freeaddrinfo(result);
	}
	catch (...)
	{
		freeaddrinfo(result);
		throw;
	}
}

// Copy constructor and assignment
SocketAddress::SocketAddress(const SocketAddress &other) : _family(other._family), _addrLen(other._addrLen)
{
	std::memcpy(&_addr, &other._addr, sizeof(_addr));
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

SocketAddress::~SocketAddress()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

SocketAddress &SocketAddress::operator=(SocketAddress const &rhs)
{
	if (this != &rhs)
	{
		_family = rhs._family;
		_addrLen = rhs._addrLen;
		std::memcpy(&_addr, &rhs._addr, sizeof(_addr));
	}
	return *this;
}

bool SocketAddress::operator==(SocketAddress const &rhs) const
{
	return _family == rhs._family && _addrLen == rhs._addrLen && std::memcmp(&_addr, &rhs._addr, sizeof(_addr)) == 0;
}

bool SocketAddress::operator!=(SocketAddress const &rhs) const
{
	return !(*this == rhs);
}

std::ostream &operator<<(std::ostream &o, SocketAddress const &i)
{
	o << "SocketAddress(" << i.getHost() << ":" << i.getPort() << ")";
	return o;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

/*
** --------------------------------- VALIDATORS --------------------------------
*/

bool SocketAddress::isIPv4() const
{
	return _family == AF_INET;
}

bool SocketAddress::isIPv6() const
{
	return _family == AF_INET6;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

const struct sockaddr *SocketAddress::getSockAddr() const
{
	return &_addr._addr_generic;
}

std::string SocketAddress::getHostString() const
{
	if (isIPv4())
	{
		return IPAddressParser::ipv4ToString(_addr._addr4.sin_addr.s_addr);
	}
	else if (isIPv6())
	{
		return IPAddressParser::ipv6ToString(_addr._addr6.sin6_addr);
	}
	return "unknown";
}

std::string SocketAddress::getPortString() const
{
	std::stringstream ss;
	ss << getPort();
	return ss.str();
}

const struct sockaddr_in &SocketAddress::getIPV4() const
{
	if (isIPv4())
		return _addr._addr4;
	{
		std::stringstream ss;
		ss << "SocketAddress is not an IPv4 address";
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

const struct sockaddr_in6 &SocketAddress::getIPV6() const
{
	if (isIPv6())
		return _addr._addr6;
	{
		std::stringstream ss;
		ss << "SocketAddress is not an IPv6 address";
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

const struct sockaddr_in &SocketAddress::get() const
{
	return getIPV4();
}

int SocketAddress::getFamily() const
{
	return _family;
}

unsigned short SocketAddress::getPort() const
{
	if (isIPv4())
	{
		return ntohs(_addr._addr4.sin_port);
	}
	else if (isIPv6())
	{
		return ntohs(_addr._addr6.sin6_port);
	}
	return 0;
}
std::string SocketAddress::getHost() const
{
	return _host;
}

socklen_t SocketAddress::getAddrLen() const
{
	return _addrLen;
}

socklen_t SocketAddress::getSize() const
{
	return _addrLen;
}

void SocketAddress::setFamily(int family)
{
	_family = family;
}

void SocketAddress::setPort(int port)
{
	_port = port;
}

void SocketAddress::setHost(const std::string &host)
{
	_host = host;
}

void SocketAddress::setAddrLen(socklen_t addrLen)
{
	_addrLen = addrLen;
}

/* ************************************************************************** */
