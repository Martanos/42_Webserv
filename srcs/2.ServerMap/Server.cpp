#include "../../includes/Core/Server.hpp"
#include "../../includes/HTTP/Constants.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Server::Server()
	: _config(NULL)
{
<<<<<<< HEAD
	_serverName = "";
	_host = SERVER::DEFAULT_HOST;
	_port = SERVER::DEFAULT_PORT;
	_locations = std::map<std::string, Location>();
}

Server::Server(const std::string &serverName, const std::string &host, const unsigned short &port,
			   const ServerConfig* serverConfig)
{
	_serverName = serverName;
	_host = host;
	_port = port;
	_config = serverConfig;
	_locations = std::map<std::string, Location>();
=======
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
>>>>>>> ConfigParserRefactor
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
<<<<<<< HEAD
		_serverName = rhs._serverName;
		_host = rhs._host;
		_port = rhs._port;
		_config = rhs._config;
		_locations = rhs._locations;
=======
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
>>>>>>> ConfigParserRefactor
	}
	return *this;
}

std::ostream &operator<<(std::ostream &o, Server const &i)
{
	o << "--------------------------------" << std::endl;
	o << "Server names: ";
	for (TrieTree<std::string>::const_iterator it = i.getServerNames().begin(); it != i.getServerNames().end(); ++it)
		o << *it << " ";
	o << std::endl;
	o << "Hosts/ports: ";
	for (std::vector<SocketAddress>::const_iterator it = i.getSocketAddresses().begin();
		 it != i.getSocketAddresses().end(); ++it)
		o << it->getHost() << ":" << it->getPort() << " ";
	o << std::endl;
	o << "Root: " << i.getRootPath() << std::endl;
	o << "Indexes: ";
	for (TrieTree<std::string>::const_iterator it = i.getIndexes().begin(); it != i.getIndexes().end(); ++it)
		o << *it << " ";
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
	for (TrieTree<Location>::const_iterator it = i.getLocations().begin(); it != i.getLocations().end(); ++it)
		o << it->getPath() << " ";
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

<<<<<<< HEAD
bool Server::getKeepAlive() const
{
	if (_config)
		return _config->getKeepAlive();
	return SERVER::DEFAULT_KEEP_ALIVE; // Need to define this constant
=======
const TrieTree<std::string> &Server::getServerNames() const
{
	return _serverNames;
>>>>>>> ConfigParserRefactor
}

const std::vector<SocketAddress> &Server::getSocketAddresses() const
{
	return _sockets;
}

const std::string &Server::getRootPath() const
{
	return _rootPath;
}

<<<<<<< HEAD
const unsigned short &Server::getPort() const
{
	return _port;
}

const std::string &Server::getRoot() const
{
	if (_config)
		return _config->getRoot();
	static const std::string defaultRoot = SERVER::DEFAULT_ROOT;
	return defaultRoot;
}

const std::vector<std::string> &Server::getIndexes() const
=======
const TrieTree<std::string> &Server::getIndexes() const
>>>>>>> ConfigParserRefactor
{
	if (_config)
		return _config->getIndexes();
	static const std::vector<std::string> defaultIndexes(1, SERVER::DEFAULT_INDEX);
	return defaultIndexes;
}

<<<<<<< HEAD
bool Server::getAutoindex() const
{
	if (_config)
		return _config->getAutoindex();
	return SERVER::DEFAULT_AUTOINDEX;
}

=======
>>>>>>> ConfigParserRefactor
double Server::getClientMaxBodySize() const
{
	if (_config)
		return _config->getClientMaxBodySize();
	return SERVER::DEFAULT_CLIENT_MAX_BODY_SIZE;
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
	if (_config)
		return _config->getStatusPages();
	static const std::map<int, std::string> emptyMap;
	return emptyMap;
}

// Returns longest prefix match location null if no match found
const Location *Server::getLocation(const std::string &path) const
{
	return _locations.findLongestPrefix(path);
}

const TrieTree<Location> &Server::getLocations() const
{
	return _locations;
}

<<<<<<< HEAD
const std::string Server::getStatusPage(const int &status) const
{
	if (_config)
	{
		const std::map<int, std::string>& statusPages = _config->getStatusPages();
		if (statusPages.find(status) == statusPages.end())
			return DefaultStatusMap::getStatusInfo(status);
		return statusPages.at(status);
	}
	return DefaultStatusMap::getStatusInfo(status);
}

const Location Server::getLocation(const std::string &path) const
{
	if (_locations.find(path) == _locations.end())
		return Location();
	return _locations.at(path);
}

=======
>>>>>>> ConfigParserRefactor
/*
** --------------------------------- SETTERS ---------------------------------
*/

<<<<<<< HEAD
void Server::setServerName(const std::string &serverName)
{
	_serverName = serverName;
}

void Server::setHost(const std::string &host)
{
	_host = host;
}

void Server::setPort(const unsigned short &port)
{
	_port = port;
}

void Server::setLocations(const std::map<std::string, Location> &locations)
=======
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
	if (!hasLocation(location.getPath()))
	{
		_locations.insert(location.getPath(), location);
		_modified = true;
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
>>>>>>> ConfigParserRefactor
{
	_clientMaxUriSize = clientMaxUriSize;
	_modified = true;
}

<<<<<<< HEAD
/*
** --------------------------------- METHODS ----------------------------------
*/

void Server::addLocation(const Location &location)
=======
void Server::setRoot(const std::string &root)
{
	_rootPath = root;
	_modified = true;
}

void Server::setAutoindex(const bool &autoindex)
>>>>>>> ConfigParserRefactor
{
	_autoIndex = autoindex;
	_modified = true;
}

<<<<<<< HEAD
=======
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

>>>>>>> ConfigParserRefactor
/* ************************************************************************** */
