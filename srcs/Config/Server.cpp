#include "../../includes/Config/Server.hpp"
#include "../../includes/Http/HTTP.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Server::Server()
{
	reset();
}

Server::Server(const Server &src)
{
	*this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Server::~Server()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

// Deep copy assignment operator
Server &Server::operator=(Server const &rhs)
{
	if (this != &rhs)
	{
		// Identifier members
		_serverNames = rhs._serverNames;
		_sockets = rhs._sockets;

		// Identifier flags
		_hasServerNameDirective = rhs._hasServerNameDirective;
		_hasSocketDirective = rhs._hasSocketDirective;

		// Directives members
		_rootPath = rhs._rootPath;
		_autoIndexValue = rhs._autoIndexValue;
		_cgiPath = rhs._cgiPath;
		_clientMaxBodySize = rhs._clientMaxBodySize;
		_keepAliveValue = rhs._keepAliveValue;
		_redirect = rhs._redirect;
		_indexes = rhs._indexes;
		_statusPaths = rhs._statusPaths;
		_allowedMethods = rhs._allowedMethods;

		// Directive flags
		_hasRootPathDirective = rhs._hasRootPathDirective;
		_hasAutoIndexDirective = rhs._hasAutoIndexDirective;
		_hasCgiPathDirective = rhs._hasCgiPathDirective;
		_hasClientMaxBodySizeDirective = rhs._hasClientMaxBodySizeDirective;
		_hasKeepAliveDirective = rhs._hasKeepAliveDirective;
		_hasRedirectDirective = rhs._hasRedirectDirective;
		_hasIndexDirective = rhs._hasIndexDirective;
		_hasStatusPathDirective = rhs._hasStatusPathDirective;
		_hasAllowedMethodsDirective = rhs._hasAllowedMethodsDirective;

		// Location members
		_locations = rhs._locations;

		// Location flags
		_hasLocationBlocks = rhs._hasLocationBlocks;
	}
	return *this;
}

std::ostream &operator<<(std::ostream &o, Server const &i)
{
	o << "--------------------------------" << std::endl;
	if (i.hasServerNameDirective())
	{
		o << "Server names: ";
		if (!i.getServerNames().isEmpty())
		{
			for (TrieTree<std::string>::const_iterator it = i.getServerNames().begin(); it != i.getServerNames().end();
				 ++it)
				o << *it << " ";
		}
		o << std::endl;
	}
	else
		o << "No server name defined!" << std::endl;

	if (!i.hasSocketDirective())
	{
		o << "Hosts/ports: ";
		for (std::vector<SocketAddress>::const_iterator it = i.getSocketAddresses().begin();
			 it != i.getSocketAddresses().end(); ++it)
			o << it->getHost() << ":" << it->getPort() << " ";
		o << std::endl;
	}
	else
		o << "No socket address defined!" << std::endl;

	if (i.hasRootPathDirective())
	{
		o << "Root: " << i.getRootPath() << std::endl;
	}
	else
		o << "No root defined!" << std::endl;

	if (i.hasIndexDirective())
	{
		o << "Indexes: ";
		if (!i.getIndexes().isEmpty())
		{
			std::vector<std::string> indexes = i.getIndexes().getAllKeys();
			for (std::vector<std::string>::const_iterator it = indexes.begin(); it != indexes.end(); ++it)
				o << *it << " ";
		}
		o << std::endl;
	}
	else
		o << "No indexes defined!" << std::endl;

	if (i.hasAutoIndexDirective())
	{
		o << "Autoindex: " << (i.getAutoIndexValue() ? "true" : "false") << std::endl;
	}
	else
		o << "No autoindex defined!" << std::endl;

	if (i.hasClientMaxBodySizeDirective())
	{
		o << "Client max body size: " << i.getClientMaxBodySize() << std::endl;
	}
	else
		o << "No client max body size defined using default value of :" << HTTP::DEFAULT_CLIENT_MAX_BODY_SIZE
		  << std::endl;

	if (i.hasStatusPathDirective())
	{
		o << "Status pages: ";
		for (std::map<int, std::string>::const_iterator it = i.getStatusPaths().begin(); it != i.getStatusPaths().end();
			 ++it)
			o << it->first << ": " << it->second << " ";
		o << std::endl;
	}
	else
		o << "No status pages defined using default values!" << std::endl;

	if (i.hasKeepAliveDirective())
	{
		o << "Keep alive: " << (i.getKeepAliveValue() ? "true" : "false") << std::endl;
	}
	else
		o << "No keep alive defined using default value of: " << (HTTP::DEFAULT_KEEP_ALIVE ? "true" : "false")
		  << std::endl;

	if (i.hasLocationBlocks())
	{
		for (std::vector<Location> locations = i.getLocations().getAllValues(); !locations.empty();
			 locations = i.getLocations().getAllValues())
		{
			o << "Locations: ";
			for (std::vector<Location>::const_iterator it = locations.begin(); it != locations.end(); ++it)
				o << it->getLocationPath() << " ";
			o << std::endl;
		}
	}
	else
		o << "No location blocks defined!" << std::endl;
	o << "--------------------------------" << std::endl;
	return o;
}

/*
** --------------------------------- IDENTIFIERS ---------------------------------
*/

// Identifier flag investigators

bool Server::hasServerNameDirective() const
{
	return _hasServerNameDirective;
}
bool Server::hasSocketDirective() const
{
	return _hasSocketDirective;
}

// Identifier investigators

bool Server::hasServerName(const std::string &serverName) const
{
	return _serverNames.contains(serverName);
}

bool Server::hasSocketAddress(const SocketAddress &socketAddress) const
{
	return std::find(_sockets.begin(), _sockets.end(), socketAddress) != _sockets.end();
}

// Identifier accessors

const TrieTree<std::string> &Server::getServerNames() const
{
	return _serverNames;
}
const std::vector<SocketAddress> &Server::getSocketAddresses() const
{
	return _sockets;
}

// Identifier mutators

void Server::insertServerName(const std::string &serverName)
{
	if (!hasServerName(serverName))
	{
		_serverNames.insert(serverName, serverName);
		_hasServerNameDirective = true;
	}
}

void Server::insertSocketAddress(const SocketAddress &socketAddress)
{
	if (!hasSocketAddress(socketAddress))
	{
		_sockets.push_back(socketAddress);
		_hasSocketDirective = true;
	}
}

/*
** --------------------------------- LOCATIONS ---------------------------------
*/

// Location flag investigators
bool Server::hasLocationBlocks() const
{
	return _hasLocationBlocks;
}

// Location investigators

// Exact match location
bool Server::hasLocation(const std::string &path) const
{
	return _locations.contains(path);
}

// Longest prefix match location
bool Server::hasLongestPrefixLocation(const std::string &path) const
{
	const Location *location = _locations.findLongestPrefix(path);
	return location != NULL;
}

// Location accessors

// Returns longest prefix match location null if no match found
const Location *Server::getLocation(const std::string &path) const
{
	Logger::debug("Server::getLocation: Looking for path: " + path, __FILE__, __LINE__, __PRETTY_FUNCTION__);
	const Location *location = _locations.findLongestPrefix(path);
	if (location)
	{
		Logger::debug("Server::getLocation: Found location: " + location->getLocationPath(), __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);
	}
	else
	{
		Logger::debug("Server::getLocation: No location found for path: " + path, __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);
	}
	return location;
}

const TrieTree<Location> &Server::getLocations() const
{
	return _locations;
}

TrieTree<Location> &Server::getLocations()
{
	return _locations;
}

// Location Mutators
void Server::insertLocation(const Location &location)
{
	Logger::debug("Server::insertLocation: Adding location: " + location.getLocationPath(), __FILE__, __LINE__,
				  __PRETTY_FUNCTION__);
	if (!hasLocation(location.getLocationPath()))
	{
		_locations.insert(location.getLocationPath(), location);
		_hasLocationBlocks = true;
		Logger::debug("Server::insertLocation: Location added successfully", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	else
	{
		Logger::debug("Server::insertLocation: Location already exists", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

/*
** --------------------------------- SERVER UTILS ---------------------------------
*/

bool Server::wasModified() const
{
	return _hasServerNameDirective || _hasSocketDirective || _hasRootPathDirective || _hasIndexDirective ||
		   _hasAutoIndexDirective || _hasClientMaxBodySizeDirective || _hasStatusPathDirective ||
		   _hasKeepAliveDirective || _hasLocationBlocks;
}

void Server::reset()
{
	// Identifier members
	_serverNames.clear();
	_sockets.clear();

	// Identifier flags
	_hasServerNameDirective = false;
	_hasSocketDirective = false;

	// Directive members
	_rootPath.clear();
	_indexes = TrieTree<std::string>();
	_autoIndexValue = HTTP::DEFAULT_AUTOINDEX;
	_clientMaxBodySize = HTTP::DEFAULT_CLIENT_MAX_BODY_SIZE;
	_statusPaths.clear();
	_keepAliveValue = HTTP::DEFAULT_KEEP_ALIVE;

	// Directive flags
	_hasRootPathDirective = false;
	_hasIndexDirective = false;
	_hasAutoIndexDirective = false;
	_hasClientMaxBodySizeDirective = false;
	_hasStatusPathDirective = false;
	_hasKeepAliveDirective = false;

	// Location blocks
	_locations = TrieTree<Location>();

	// Location flags
	_hasLocationBlocks = false;
}

/* ************************************************************************** */
