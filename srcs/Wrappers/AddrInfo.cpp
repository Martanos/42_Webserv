#include "Wrapper/AddrInfo.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

AddrInfo::AddrInfo() : _result(NULL)
{
}

AddrInfo::AddrInfo(const std::string &host, const std::string &port, int family) : _result(NULL)
{
	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(host.c_str(), port.c_str(), &hints, &_result);
	if (status != 0)
	{
		std::stringstream ss;
		ss << "getaddrinfo failed: " << gai_strerror(status);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
}

AddrInfo::AddrInfo(const AddrInfo &src) : _result(NULL)
{
	if (this != &src)
	{
		if (src._result)
		{
			this->_result = src._result;
		}
	}
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

AddrInfo::~AddrInfo()
{
	if (_result)
	{
		freeaddrinfo(_result);
	}
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

AddrInfo &AddrInfo::operator=(AddrInfo const &rhs)
{
	if (this != &rhs)
	{
		this->_result = rhs._result;
	}
	return *this;
}

std::ostream &operator<<(std::ostream &o, AddrInfo const &i)
{
	o << "AddrInfo: " << i.get();
	return o;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

const struct addrinfo *AddrInfo::get() const
{
	return _result;
}

const struct addrinfo *AddrInfo::operator->() const
{
	return _result;
}

const struct addrinfo *AddrInfo::begin() const
{
	return _result;
}

const struct addrinfo *AddrInfo::next(const struct addrinfo *current) const
{
	return current ? current->ai_next : NULL;
}

/* ************************************************************************** */
