#include "../../includes/SocketAddress.hpp"
#include "../../includes/IPAddressParser.hpp"
#include "../../includes/Logger.hpp"
#include <cstddef>
#include <sstream>

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
** ------------------------------- CONSTRUCTOR --------------------------------
*/

SocketAddress::SocketAddress() : _addrLen(sizeof(struct sockaddr_in)), _family(AF_INET), _host(""), _port(0)
{
	std::memset(&_storage, 0, sizeof(_storage));
	struct sockaddr_in *addr4 = _getSockAddrIn();
	addr4->sin_family = AF_INET;
	addr4->sin_addr.s_addr = INADDR_ANY;
	addr4->sin_port = 0;
}

// COMPATIBILITY: Keep old constructor signature (const int port, const std::string ip)
SocketAddress::SocketAddress(const int port, const std::string ip)
	: _addrLen(0), _family(AF_UNSPEC), _host(ip), _port(port)
{
	*this = SocketAddress(ip, static_cast<unsigned short>(port));
}

SocketAddress::SocketAddress(const std::string &host, unsigned short port)
	: _addrLen(0), _family(AF_UNSPEC), _host(host), _port(port)
{
	std::memset(&_storage, 0, sizeof(_storage));

	// Handle special cases first (faster path)
	if (host == "0.0.0.0" || host.empty())
	{
		struct sockaddr_in *addr4 = _getSockAddrIn();
		addr4->sin_family = AF_INET;
		addr4->sin_port = htons(port);
		addr4->sin_addr.s_addr = INADDR_ANY;
		_family = AF_INET;
		_addrLen = sizeof(struct sockaddr_in);
		_updateCachedValues();
		return;
	}

	if (host == "::")
	{
		struct sockaddr_in6 *addr6 = _getSockAddrIn6();
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = htons(port);
		addr6->sin6_addr = in6addr_any;
		_family = AF_INET6;
		_addrLen = sizeof(struct sockaddr_in6);
		_updateCachedValues();
		return;
	}

	// Use getaddrinfo for hostname resolution and validation
	struct addrinfo hints, *result = NULL;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // Allow both IPv4 and IPv6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV; // Try numeric first

	std::ostringstream portStr;
	portStr << port;

	int status = getaddrinfo(host.c_str(), portStr.str().c_str(), &hints, &result);
	if (status != 0)
	{
		// If numeric parsing failed, try hostname resolution
		hints.ai_flags = 0;
		status = getaddrinfo(host.c_str(), portStr.str().c_str(), &hints, &result);

		if (status != 0)
		{
			std::stringstream ss;
			ss << "getaddrinfo failed for host '" << host << "': " << gai_strerror(status);
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
			break; // Prefer IPv4
		}
	}

	// Copy the address to our storage
	try
	{
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
			std::stringstream ss;
			ss << "Address too large for storage: " << preferred->ai_addrlen;
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error("Address too large for storage");
		}
		freeaddrinfo(result);
	}
	catch (...)
	{
		freeaddrinfo(result);
		std::stringstream ss;
		ss << "Failed to copy address to storage: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw;
	}
}

// Copy constructor
SocketAddress::SocketAddress(const SocketAddress &src)
	: _addrLen(src._addrLen), _family(src._family), _host(src._host), _port(src._port)
{
	std::memcpy(&_storage, &src._storage, sizeof(_storage));
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
	return _family == rhs._family && _addrLen == rhs._addrLen && std::memcmp(&_storage, &rhs._storage, _addrLen) == 0;
}

bool SocketAddress::operator!=(const SocketAddress &rhs) const
{
	return !(*this == rhs);
}

bool SocketAddress::operator<(const SocketAddress &rhs) const
{
	if (_family != rhs._family)
		return _family < rhs._family;
	if (_addrLen != rhs._addrLen)
		return _addrLen < rhs._addrLen;
	return std::memcmp(&_storage, &rhs._storage, _addrLen) < 0;
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

const struct sockaddr *SocketAddress::getSockAddr() const
{
	return _getSockAddr();
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

// COMPATIBILITY: Keep original method name with capital V
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

// COMPATIBILITY: Keep original method name with capital V
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

// COMPATIBILITY: Keep original get() method
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

/*
** --------------------------------- SETTERS (DEPRECATED) ---------------------
*/

void SocketAddress::setFamily(int family)
{
	_family = family;
	_getSockAddr()->sa_family = family;
}

void SocketAddress::setPort(int port)
{
	_port = port;
	if (_family == AF_INET)
	{
		_getSockAddrIn()->sin_port = htons(port);
	}
	else if (_family == AF_INET6)
	{
		_getSockAddrIn6()->sin6_port = htons(port);
	}
}

void SocketAddress::setHost(const std::string &host)
{
	_host = host;
}

void SocketAddress::setAddrLen(socklen_t addrLen)
{
	_addrLen = addrLen;
}

/*
** ------------------------------- STATIC METHODS ----------------------------
*/

SocketAddress SocketAddress::createIPv4(const std::string &host, unsigned short port)
{
	SocketAddress addr;
	struct sockaddr_in *addr4 = addr._getSockAddrIn();

	addr4->sin_family = AF_INET;
	addr4->sin_port = htons(port);

	if (host == "0.0.0.0" || host.empty())
	{
		addr4->sin_addr.s_addr = INADDR_ANY;
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
		addr4->sin_addr.s_addr = ipAddr;
	}

	addr._family = AF_INET;
	addr._addrLen = sizeof(struct sockaddr_in);
	addr._host = host;
	addr._updateCachedValues();

	return addr;
}

SocketAddress SocketAddress::createIPv6(const std::string &host, unsigned short port)
{
	SocketAddress addr;
	struct sockaddr_in6 *addr6 = addr._getSockAddrIn6();

	addr6->sin6_family = AF_INET6;
	addr6->sin6_port = htons(port);

	if (host == "::" || host.empty())
	{
		addr6->sin6_addr = in6addr_any;
	}
	else
	{
		if (!IPAddressParser::parseIPv6(host, addr6->sin6_addr))
		{
			throw std::runtime_error("Invalid IPv6 address: " + host);
		}
	}

	addr._family = AF_INET6;
	addr._addrLen = sizeof(struct sockaddr_in6);
	addr._host = host;
	addr._updateCachedValues();

	return addr;
}

SocketAddress SocketAddress::createAuto(const std::string &host, unsigned short port)
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
		// For hostname resolution, use the main constructor
		return SocketAddress(host, port);
	}
}

SocketAddress SocketAddress::createFromStorage(const struct sockaddr_storage &storage, socklen_t addrLen)
{
	SocketAddress addr;
	if (addrLen <= sizeof(addr._storage))
	{
		std::memcpy(&addr._storage, &storage, addrLen);
		addr._addrLen = addrLen;
		addr._family = addr._getSockAddr()->sa_family;
		addr._updateCachedValues();
	}
	else
	{
		throw std::runtime_error("Address length exceeds storage capacity");
	}
	return addr;
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