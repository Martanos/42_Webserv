#include "../../includes/Core/Server.hpp"
#include "../../includes/HTTP/HTTP.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Server::Server()
{
	_serverNames = TrieTree<std::string>();
	_sockets = std::vector<SocketAddress>();
	_rootPath = std::string();
	_indexes = TrieTree<std::string>();
	_autoIndex = HTTP::DEFAULT_AUTOINDEX;
	_clientMaxUriSize = HTTP::MAX_URI_LINE_SIZE;
	_clientMaxHeadersSize = HTTP::MAX_HEADERS_SIZE;
	_clientMaxBodySize = HTTP::MAX_BODY_SIZE;
	_statusPages = std::map<int, std::string>();
	_locations = TrieTree<Location>();
	_keepAlive = HTTP::DEFAULT_KEEP_ALIVE;

	// Flags
	_modified = false;
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
		_serverNames = rhs._serverNames;
		_sockets = rhs._sockets;
		_rootPath = rhs._rootPath;
		_indexes = rhs._indexes;
		_autoIndex = rhs._autoIndex;
		_clientMaxUriSize = rhs._clientMaxUriSize;
		_clientMaxHeadersSize = rhs._clientMaxHeadersSize;
		_clientMaxBodySize = rhs._clientMaxBodySize;
		_statusPages = rhs._statusPages;
		_locations = rhs._locations;
		_keepAlive = rhs._keepAlive;
		_modified = rhs._modified;
	}
	return *this;
}

std::ostream &operator<<(std::ostream &o, Server const &i)
{
	o << "--------------------------------" << std::endl;
	o << "Server names: ";
	if (!i.getServerNames().isEmpty())
	{
		std::vector<std::string> names = i.getServerNames().getAllKeys();
		for (std::vector<std::string>::const_iterator it = names.begin(); it != names.end(); ++it)
			o << *it << " ";
	}
	o << std::endl;
	o << "Hosts/ports: ";
	for (std::vector<SocketAddress>::const_iterator it = i.getSocketAddresses().begin();
		 it != i.getSocketAddresses().end(); ++it)
		o << it->getHost() << ":" << it->getPort() << " ";
	o << std::endl;
	o << "Root: " << i.getRootPath() << std::endl;
	o << "Indexes: ";
	if (!i.getIndexes().isEmpty())
	{
		std::vector<std::string> indexes = i.getIndexes().getAllKeys();
		for (std::vector<std::string>::const_iterator it = indexes.begin(); it != indexes.end(); ++it)
			o << *it << " ";
	}
	o << std::endl;
	o << "Autoindex: " << (i.isAutoIndex() ? "true" : "false") << std::endl;
	o << "Client max uri size: " << i.getClientMaxUriSize() << std::endl;
	o << "Client max headers size: " << i.getClientMaxHeadersSize() << std::endl;
	o << "Client max body size: " << i.getClientMaxBodySize() << std::endl;
	o << "Keep alive: " << (i.isKeepAlive() ? "true" : "false") << std::endl;
	o << "Status pages: ";
	for (std::map<int, std::string>::const_iterator it = i.getStatusPages().begin(); it != i.getStatusPages().end();
		 ++it)
		o << it->first << ": " << it->second << " ";
	o << std::endl;
	o << "Locations: ";
	if (!i.getLocations().isEmpty())
	{
		std::vector<Location> locations = i.getLocations().getAllValues();
		for (std::vector<Location>::const_iterator it = locations.begin(); it != locations.end(); ++it)
			o << it->getPath() << " ";
		for (std::vector<Location>::const_iterator it = locations.begin(); it != locations.end(); ++it)
			o << std::endl
			  << *it;
	}
	o << std::endl;
	o << "--------------------------------" << std::endl;
	return o;
}

/*
** --------------------------------- INVESTIGATORS ---------------------------------
*/

bool Server::hasServerName(const std::string &serverName) const
{
	return _serverNames.contains(serverName);
}

bool Server::hasSocketAddress(const SocketAddress &socketAddress) const
{
	return std::find(_sockets.begin(), _sockets.end(), socketAddress) != _sockets.end();
}

bool Server::hasIndex(const std::string &index) const
{
	return _indexes.contains(index);
}

// Exact match location
bool Server::hasLocation(const std::string &path) const
{
	return _locations.contains(path);
}

bool Server::hasStatusPage(int status) const
{
	return _statusPages.find(status) != _statusPages.end();
}

bool Server::hasRootPath() const
{
	return !_rootPath.empty();
}

bool Server::isAutoIndex() const
{
	return _autoIndex;
}

bool Server::isKeepAlive() const
{
	return _keepAlive;
}

bool Server::isModified() const
{
	return _modified;
}

/*
** --------------------------------- GETTERS ---------------------------------
*/

const TrieTree<std::string> &Server::getServerNames() const
{
	return _serverNames;
}

const std::vector<SocketAddress> &Server::getSocketAddresses() const
{
	return _sockets;
}

const std::string &Server::getRootPath() const
{
	return _rootPath;
}

const TrieTree<std::string> &Server::getIndexes() const
{
	return _indexes;
}

double Server::getClientMaxBodySize() const
{
	return _clientMaxBodySize;
}

double Server::getClientMaxHeadersSize() const
{
	return _clientMaxHeadersSize;
}

double Server::getClientMaxUriSize() const
{
	return _clientMaxUriSize;
}

const std::string &Server::getStatusPath(int status) const
{
	if (!hasStatusPage(status))
		throw std::out_of_range("Server: Status page not found");
	return _statusPages.at(status);
}

const std::map<int, std::string> &Server::getStatusPages() const
{
	return _statusPages;
}

// Returns longest prefix match location null if no match found
const Location *Server::getLocation(const std::string &path) const
{
	Logger::debug("Server::getLocation: Looking for path: " + path);
	const Location *location = _locations.findLongestPrefix(path);
	if (location)
	{
		Logger::debug("Server::getLocation: Found location: " + location->getPath());
	}
	else
	{
		Logger::debug("Server::getLocation: No location found for path: " + path);
	}
	return location;
}

const TrieTree<Location> &Server::getLocations() const
{
	return _locations;
}

/*
** --------------------------------- SETTERS ---------------------------------
*/

void Server::insertServerName(const std::string &serverName)
{
	if (!hasServerName(serverName))
	{
		_serverNames.insert(serverName, serverName);
		_modified = true;
	}
}

void Server::insertSocketAddress(const SocketAddress &socketAddress)
{
	if (!hasSocketAddress(socketAddress))
	{
		_sockets.push_back(socketAddress);
		_modified = true;
	}
}

void Server::insertIndex(const std::string &index)
{
	if (!hasIndex(index))
	{
		_indexes.insert(index, index);
		_modified = true;
	}
}

void Server::insertLocation(const Location &location)
{
	Logger::debug("Server::insertLocation: Adding location: " + location.getPath());
	if (!hasLocation(location.getPath()))
	{
		_locations.insert(location.getPath(), location);
		_modified = true;
		Logger::debug("Server::insertLocation: Location added successfully");
	}
	else
	{
		Logger::debug("Server::insertLocation: Location already exists");
	}
}

void Server::insertStatusPage(const std::string &path, const std::vector<int> &codes)
{
	for (std::vector<int>::const_iterator code_it = codes.begin(); code_it != codes.end(); ++code_it)
	{
		_statusPages.insert(std::make_pair(*code_it, path));
		_modified = true;
	}
}

void Server::setKeepAlive(const bool &keepAlive)
{
	_keepAlive = keepAlive;
	_modified = true;
}

void Server::setClientMaxBodySize(const double &clientMaxBodySize)
{
	_clientMaxBodySize = clientMaxBodySize;
	_modified = true;
}

void Server::setClientMaxHeadersSize(const double &clientMaxHeadersSize)
{
	_clientMaxHeadersSize = clientMaxHeadersSize;
	_modified = true;
}

void Server::setClientMaxUriSize(const double &clientMaxUriSize)
{
	_clientMaxUriSize = clientMaxUriSize;
	_modified = true;
}

void Server::setRoot(const std::string &root)
{
	_rootPath = root;
	_modified = true;
}

void Server::setAutoindex(const bool &autoindex)
{
	_autoIndex = autoindex;
	_modified = true;
}

void Server::reset()
{
	_serverNames.clear();
	_sockets.clear();
	_rootPath = std::string();
	_indexes.clear();
	_autoIndex = HTTP::DEFAULT_AUTOINDEX;
	_clientMaxUriSize = HTTP::MAX_URI_LINE_SIZE;
	_clientMaxHeadersSize = HTTP::MAX_HEADERS_SIZE;
	_clientMaxBodySize = HTTP::MAX_BODY_SIZE;
	_statusPages.clear();
	_locations.clear();
	_keepAlive = HTTP::DEFAULT_KEEP_ALIVE;
	_modified = false;
}

/* ************************************************************************** */
