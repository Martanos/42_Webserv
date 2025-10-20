#include "../../includes/Server.hpp"
#include "../../includes/Constants.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Server::Server()
{
	_serverNames = TrieTree<std::string>();
	_hosts_ports = std::vector<std::pair<std::string, unsigned short> >();
	_root = "";
	_indexes = TrieTree<std::string>();
	_autoindex = HTTP::DEFAULT_AUTOINDEX;
	_clientMaxUriSize = HTTP::MAX_URI_LINE_SIZE;
	_clientMaxHeadersSize = HTTP::MAX_HEADERS_SIZE;
	_clientMaxBodySize = HTTP::MAX_BODY_SIZE;
	_statusPages = std::map<int, std::string>();
	_locations = TrieTree<Location>();
	_keepAlive = HTTP::DEFAULT_KEEP_ALIVE;
	_keepAliveSet = false;
	_autoindexSet = false;
	_clientMaxUriSizeSet = false;
	_clientMaxHeadersSizeSet = false;
	_clientMaxBodySizeSet = false;
	_rootSet = false;
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
		_hosts_ports = rhs._hosts_ports;
		_root = rhs._root;
		_indexes = rhs._indexes;
		_autoindex = rhs._autoindex;
		_clientMaxUriSize = rhs._clientMaxUriSize;
		_clientMaxHeadersSize = rhs._clientMaxHeadersSize;
		_clientMaxBodySize = rhs._clientMaxBodySize;
		_statusPages = rhs._statusPages;
		_locations = rhs._locations;
		_keepAlive = rhs._keepAlive;
		_keepAliveSet = rhs._keepAliveSet;
		_autoindexSet = rhs._autoindexSet;
		_clientMaxUriSizeSet = rhs._clientMaxUriSizeSet;
		_clientMaxHeadersSizeSet = rhs._clientMaxHeadersSizeSet;
		_clientMaxBodySizeSet = rhs._clientMaxBodySizeSet;
		_rootSet = rhs._rootSet;
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
	for (std::vector<std::pair<std::string, unsigned short> >::const_iterator it = i.getHosts_ports().begin();
		 it != i.getHosts_ports().end(); ++it)
		o << it->first << ":" << it->second << " ";
	o << std::endl;
	o << "Root: " << i.getRoot() << std::endl;
	o << "Indexes: ";
	for (TrieTree<std::string>::const_iterator it = i.getIndexes().begin(); it != i.getIndexes().end(); ++it)
		o << *it << " ";
	o << std::endl;
	o << "Autoindex: " << (i.getAutoindex() ? "true" : "false") << std::endl;
	o << "Client max body size: " << i.getClientMaxBodySize() << std::endl;
	o << "Keep alive: " << (i.getKeepAlive() ? "true" : "false") << std::endl;
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
** --------------------------------- GETTERS ---------------------------------
*/

// Returns pointer to server name if found, NULL otherwise
const std::string *Server::getServerName(const std::string &serverName) const
{
	return _serverNames.find(serverName);
}

const TrieTree<std::string> &Server::getServerNames() const
{
	return _serverNames;
}

const std::pair<std::string, unsigned short> *Server::getHostPort(const std::string &host,
																  const unsigned short &port) const
{
	std::pair<std::string, unsigned short> host_port(host, port);
	if (std::find(_hosts_ports.begin(), _hosts_ports.end(), host_port) == _hosts_ports.end())
		return NULL;
	return &*std::find(_hosts_ports.begin(), _hosts_ports.end(), host_port);
}

const std::vector<std::pair<std::string, unsigned short> > &Server::getHosts_ports() const
{
	return _hosts_ports;
}

const std::string &Server::getRoot() const
{
	return _root;
}

const std::string *Server::getIndex(const std::string &index) const
{
	return _indexes.find(index);
}

const TrieTree<std::string> &Server::getIndexes() const
{
	return _indexes;
}

const bool Server::getAutoindex() const
{
	return _autoindex ? true : false;
}

const double &Server::getClientMaxBodySize() const
{
	return _clientMaxBodySize;
}

const double &Server::getClientMaxHeadersSize() const
{
	return _clientMaxHeadersSize;
}

const double &Server::getClientMaxUriSize() const
{
	return _clientMaxUriSize;
}
const std::string *Server::getStatusPage(const int &status) const
{
	if (_statusPages.find(status) == _statusPages.end())
		return NULL;
	return &_statusPages.find(status)->second;
}

const std::map<int, std::string> &Server::getStatusPages() const
{
	return _statusPages;
}

// Returns longest prefix match location
const Location *Server::getLocation(const std::string &path) const
{
	return _locations.findLongestPrefix(path);
}

const TrieTree<Location> &Server::getLocations() const
{
	return _locations;
}

const bool Server::getKeepAlive() const
{
	return _keepAlive ? true : false;
}

/*
** --------------------------------- SETTERS ---------------------------------
*/

void Server::insertServerName(const std::string &serverName)
{
	_serverNames.insert(serverName, serverName);
}

void Server::insertHostPort(const std::string &host, const unsigned short &port)
{
	_hosts_ports.push_back(std::make_pair(host, port));
}

void Server::insertIndex(const std::string &index)
{
	_indexes.insert(index, index);
}

void Server::insertLocation(const Location &location)
{
	_locations.insert(location.getPath(), location);
}

void Server::insertStatusPage(const int &status, const std::string &path)
{
	if (_statusPages.find(status) == _statusPages.end())
		_statusPages[status] = path;
	else
		std::stringstream ss;
	ss << "Status page " << status << " already exists in server";
	throw std::runtime_error(ss.str());
}

void Server::setKeepAlive(const bool &keepAlive)
{
	if (_keepAliveSet)
		throw std::runtime_error("Keep alive already set for server");
	_keepAlive = keepAlive;
	_keepAliveSet = true;
}

void Server::setClientMaxBodySize(const double &clientMaxBodySize)
{
	if (_clientMaxBodySizeSet)
		throw std::runtime_error("Client max body size already set for server");
	_clientMaxBodySizeSet = true;
	_clientMaxBodySize = clientMaxBodySize;
}

void Server::setClientMaxHeadersSize(const double &clientMaxHeadersSize)
{
	if (_clientMaxHeadersSizeSet)
		throw std::runtime_error("Client max headers size already set for server");
	_clientMaxHeadersSizeSet = true;
	_clientMaxHeadersSize = clientMaxHeadersSize;
}

void Server::setClientMaxUriSize(const double &clientMaxUriSize)
{
	if (_clientMaxUriSizeSet)
		throw std::runtime_error("Client max uri size already set for server");
	_clientMaxUriSizeSet = true;
	_clientMaxUriSize = clientMaxUriSize;
}

void Server::setRoot(const std::string &root)
{
	if (_rootSet)
		throw std::runtime_error("Root already set for server");
	_rootSet = true;
	_root = root;
}

void Server::setAutoindex(const bool &autoindex)
{
	if (_autoindexSet)
		throw std::runtime_error("Autoindex already set for server");
	_autoindexSet = true;
	_autoindex = autoindex;
}

void Server::reset()
{
	_serverNames.clear();
	_hosts_ports.clear();
	_root = "";
	_indexes.clear();
	_autoindex = HTTP::DEFAULT_AUTOINDEX;
	_clientMaxUriSize = HTTP::MAX_URI_LINE_SIZE;
	_clientMaxHeadersSize = HTTP::MAX_HEADERS_SIZE;
	_clientMaxBodySize = HTTP::MAX_BODY_SIZE;
	_statusPages.clear();
	_locations.clear();
	_keepAlive = HTTP::DEFAULT_KEEP_ALIVE;
	_keepAliveSet = false;
	_autoindexSet = false;
	_clientMaxUriSizeSet = false;
	_clientMaxHeadersSizeSet = false;
	_clientMaxBodySizeSet = false;
	_rootSet = false;
}

/* ************************************************************************** */
