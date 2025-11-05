#include "../../includes/Wrapper/SocketAddress.hpp"
#include "../../includes/Global/IPAddressParser.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include <cstddef>
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

SocketAddress::SocketAddress() : _addrLen(0), _family(AF_UNSPEC), _host("localhost"), _port(80)
{
	std::memset(&_storage, 0, sizeof(_storage));
}

SocketAddress::SocketAddress(const std::string &host, const unsigned short &port) : _addrLen(0), _family(AF_UNSPEC)
{
	*this = SocketAddress(host, StrUtils::toString(port));
}

SocketAddress::SocketAddress(const struct sockaddr_storage &storage, socklen_t addrLen)
	: _addrLen(addrLen), _family(storage.ss_family)
{
	std::memcpy(&_storage, &storage, addrLen);
	_updateCachedValues();
}

SocketAddress::SocketAddress(const std::string &host, const std::string &port) : _addrLen(0), _family(AF_UNSPEC)
{
	std::memset(&_storage, 0, sizeof(_storage));

	// Sanitize host
	if (!host.empty())
	{
		if (host[0] == '[')
		{
			size_t bracket_pos = host.find(']');
			if (bracket_pos != std::string::npos)
				_host = host.substr(1, bracket_pos - 1);
			else
				throw std::runtime_error("Malformed IPv6 address: " + host);
		}
		else
			_host = host;
	}
	else
	{
		_host = "0.0.0.0";
	}

	// Sanitize and validate port
	if (!port.empty())
	{
		errno = 0;
		char *endptr;
		long long converted_port = std::strtoll(port.c_str(), &endptr, 10);
		if (*endptr != '\0' || errno != 0 || converted_port <= 0 || converted_port > 65535)
			throw std::runtime_error("Invalid port: " + port);
		_port = static_cast<unsigned short>(converted_port);
	}
	else
	{
		_port = 80;
	}

	if (_host == "0.0.0.0")
	{
		struct sockaddr_in *addr4 = _getSockAddrIn();
		addr4->sin_family = AF_INET;
		addr4->sin_port = htons(_port);
		addr4->sin_addr.s_addr = htonl(INADDR_ANY);
		_family = AF_INET;
		_addrLen = sizeof(struct sockaddr_in);
		return;
	}

	if (_host == "::")
	{
		struct sockaddr_in6 *addr6 = _getSockAddrIn6();
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = htons(_port);
		std::memset(&addr6->sin6_addr, 0, sizeof(addr6->sin6_addr));
		_family = AF_INET6;
		_addrLen = sizeof(struct sockaddr_in6);
		return;
	}

	// Use getaddrinfo for specific addresses and hostname resolution
	struct addrinfo hints, *result = NULL;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV; // Try numeric first

	int status = getaddrinfo(_host.c_str(), port.c_str(), &hints, &result);
	if (status != 0)
	{
		// If numeric parsing failed, try hostname resolution
		hints.ai_flags = 0;
		status = getaddrinfo(_host.c_str(), port.c_str(), &hints, &result);

		if (status != 0)
		{
			std::stringstream ss;
			ss << "getaddrinfo failed for host '" << _host << "': " << gai_strerror(status);
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}
	}

	// Find preferred address (IPv4 preferred for compatibility)
	struct addrinfo *preferred = result;
	for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next)
	{
		if (rp->ai_family == AF_INET)
		{
			preferred = rp;
			break;
		}
	}

	if (preferred->ai_addrlen <= sizeof(_storage))
	{
		std::memcpy(&_storage, preferred->ai_addr, preferred->ai_addrlen);
		_addrLen = preferred->ai_addrlen;
		_family = preferred->ai_family;
		_updateCachedValues();
	}
	else
	{
		freeaddrinfo(result);
		throw std::runtime_error("Address too large for storage");
	}

	freeaddrinfo(result);
}

SocketAddress::SocketAddress(const std::string &host_port) : _addrLen(0), _family(AF_UNSPEC)
{
	if (!host_port.empty() && host_port[0] == '[')
	{
		size_t end = host_port.find(']');
		if (end == std::string::npos)
			throw std::runtime_error("Malformed IPv6 host: " + host_port);
		std::string host = host_port.substr(1, end - 1);
		std::string port = (end + 1 < host_port.size() && host_port[end + 1] == ':') ? host_port.substr(end + 2) : "";
		*this = SocketAddress(host, port);
	}
	else
	{
		size_t colon_pos = host_port.rfind(':');
		if (colon_pos == std::string::npos)
			*this = SocketAddress("0.0.0.0", host_port);
		else
			*this = SocketAddress(host_port.substr(0, colon_pos), host_port.substr(colon_pos + 1));
	}
}
// Copy constructor
SocketAddress::SocketAddress(const SocketAddress &src)
	: _addrLen(src._addrLen), _family(src._family), _host(src._host), _port(src._port)
{
	std::memcpy(&_storage, &src._storage, sizeof(_storage));
}
/*
** ------------------------------- PRIVATE HELPERS ---------------------------
*/

void SocketAddress::_updateCachedValues()
{
	if (_family == AF_INET && _addrLen >= sizeof(struct sockaddr_in))
	{
		const struct sockaddr_in *addr4 = _getSockAddrIn();
		_port = ntohs(addr4->sin_port);
	}
	else if (_family == AF_INET6 && _addrLen >= sizeof(struct sockaddr_in6))
	{
		const struct sockaddr_in6 *addr6 = _getSockAddrIn6();
		_port = ntohs(addr6->sin6_port);
	}
	else
	{
		_port = 0;
	}
}

const struct sockaddr *SocketAddress::_getSockAddr() const
{
	return reinterpret_cast<const struct sockaddr *>(&_storage);
}

struct sockaddr *SocketAddress::_getSockAddr()
{
	return reinterpret_cast<struct sockaddr *>(&_storage);
}

const struct sockaddr_in *SocketAddress::_getSockAddrIn() const
{
	return reinterpret_cast<const struct sockaddr_in *>(&_storage);
}

struct sockaddr_in *SocketAddress::_getSockAddrIn()
{
	return reinterpret_cast<struct sockaddr_in *>(&_storage);
}

const struct sockaddr_in6 *SocketAddress::_getSockAddrIn6() const
{
	return reinterpret_cast<const struct sockaddr_in6 *>(&_storage);
}

struct sockaddr_in6 *SocketAddress::_getSockAddrIn6()
{
	return reinterpret_cast<struct sockaddr_in6 *>(&_storage);
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

SocketAddress &SocketAddress::operator=(const SocketAddress &rhs)
{
	if (this != &rhs)
	{
		_addrLen = rhs._addrLen;
		_family = rhs._family;
		_host = rhs._host;
		_port = rhs._port;
		std::memcpy(&_storage, &rhs._storage, sizeof(_storage));
	}
	return *this;
}

bool SocketAddress::operator==(const SocketAddress &rhs) const
{
	return _host == rhs._host && _port == rhs._port;
}

bool SocketAddress::operator!=(const SocketAddress &rhs) const
{
	return !(*this == rhs);
}

bool SocketAddress::operator<(const SocketAddress &rhs) const
{
	return _host == rhs._host && _port < rhs._port;
}

bool SocketAddress::operator>(const SocketAddress &rhs) const
{
	return rhs < *this;
}

bool SocketAddress::operator<=(const SocketAddress &rhs) const
{
	return *this < rhs || *this == rhs;
}

bool SocketAddress::operator>=(const SocketAddress &rhs) const
{
	return *this > rhs || *this == rhs;
}

std::ostream &operator<<(std::ostream &o, const SocketAddress &i)
{
	o << i.getHost() << ":" << i.getPort();
	return o;
}

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

bool SocketAddress::isValid() const
{
	return _family != AF_UNSPEC && _addrLen > 0;
}

bool SocketAddress::isEmpty() const
{
	return _addrLen == 0 || _family == AF_UNSPEC;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

struct sockaddr_storage *SocketAddress::getSockAddr()
{
	return &_storage;
}

const struct sockaddr_storage *SocketAddress::getSockAddr() const
{
	return &_storage;
}

std::string SocketAddress::getHostString() const
{
	if (isIPv4())
	{
		return IPAddressParser::ipv4ToString(_getSockAddrIn()->sin_addr.s_addr);
	}
	else if (isIPv6())
	{
		return IPAddressParser::ipv6ToString(_getSockAddrIn6()->sin6_addr);
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
		return *_getSockAddrIn();
	else
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
		return *_getSockAddrIn6();
	else
	{
		std::stringstream ss;
		ss << "SocketAddress is not an IPv6 address";
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

int SocketAddress::getFamily() const
{
	return _family;
}

unsigned short SocketAddress::getPort() const
{
	return _port;
}

std::string SocketAddress::getHost() const
{
	return _host;
}

socklen_t SocketAddress::getAddrLen() const
{
	return _addrLen;
}

// COMPATIBILITY: Keep original getSize() method
socklen_t SocketAddress::getSize() const
{
	return _addrLen;
}

void SocketAddress::setAddrLen(socklen_t addrLen)
{
	_addrLen = addrLen;
}

/*
** --------------------------------- UTILITY ----------------------------------
*/

void SocketAddress::clear()
{
	std::memset(&_storage, 0, sizeof(_storage));
	_addrLen = 0;
	_family = AF_UNSPEC;
	_host.clear();
	_port = 0;
}

/* ************************************************************************** */