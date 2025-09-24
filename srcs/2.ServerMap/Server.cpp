#include "Server.hpp"
#include "Constants.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Server::Server()
{
	_serverName = "";
	_host = SERVER::DEFAULT_HOST;
	_port = SERVER::DEFAULT_PORT;
	_root = SERVER::DEFAULT_ROOT;
	_indexes = std::vector<std::string>(1, SERVER::DEFAULT_INDEX);
	_autoindex = SERVER::DEFAULT_AUTOINDEX;
	_clientMaxBodySize = SERVER::DEFAULT_CLIENT_MAX_BODY_SIZE;
	_statusPages = std::map<int, std::string>();
	_locations = std::map<std::string, Location>();
}

Server::Server(const std::string &serverName, const std::string &host, const unsigned short &port, const ServerConfig &serverConfig)
{
	_serverName = serverName;
	_host = host;
	_port = port;
	_root = serverConfig.getRoot();
	_indexes = serverConfig.getIndexes();
	_autoindex = serverConfig.getAutoindex();
	_clientMaxBodySize = serverConfig.getClientMaxBodySize();
	_statusPages = serverConfig.getStatusPages();
	_locations = std::map<std::string, Location>();
	_keepAlive = serverConfig.getKeepAlive();
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

Server &Server::operator=(Server const &rhs)
{
	if (this != &rhs)
	{
		_serverName = rhs._serverName;
		_host = rhs._host;
		_port = rhs._port;
		_root = rhs._root;
		_indexes = rhs._indexes;
		_autoindex = rhs._autoindex;
		_clientMaxBodySize = rhs._clientMaxBodySize;
		_statusPages = rhs._statusPages;
		_locations = rhs._locations;
		_keepAlive = rhs._keepAlive;
	}
	return *this;
}

std::ostream &operator<<(std::ostream &o, Server const &i)
{
	o << "--------------------------------" << std::endl;
	o << "Server name: " << i.getServerName() << std::endl;
	o << "Host/port: " << i.getHost() << ":" << i.getPort() << std::endl;
	o << "Root: " << i.getRoot() << std::endl;
	o << "Indexes: ";
	for (std::vector<std::string>::const_iterator it = i.getIndexes().begin(); it != i.getIndexes().end(); ++it)
		o << *it << " ";
	o << std::endl;
	o << "Autoindex: " << (i.getAutoindex() ? "true" : "false") << std::endl;
	o << "Client max body size: " << i.getClientMaxBodySize() << std::endl;
	o << "Keep alive: " << (i.getKeepAlive() ? "true" : "false") << std::endl;
	o << "Status pages: ";
	for (std::map<int, std::string>::const_iterator it = i.getStatusPages().begin(); it != i.getStatusPages().end(); ++it)
		o << it->first << ": " << it->second << " ";
	o << std::endl;
	o << "Locations: ";
	for (std::map<std::string, Location>::const_iterator it = i.getLocations().begin(); it != i.getLocations().end(); ++it)
		o << it->first << ": " << it->second << " ";
	o << std::endl;
	o << "--------------------------------" << std::endl;
	return o;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

/*
** --------------------------------- GETTERS ---------------------------------
*/

const bool &Server::getKeepAlive() const
{
	return _keepAlive;
}

const std::string &Server::getServerName() const
{
	return _serverName;
}

const std::string &Server::getHost() const
{
	return _host;
}

const unsigned short &Server::getPort() const
{
	return _port;
}

const std::string &Server::getRoot() const
{
	return _root;
}

const std::vector<std::string> &Server::getIndexes() const
{
	return _indexes;
}

const bool &Server::getAutoindex() const
{
	return _autoindex;
}

const double &Server::getClientMaxBodySize() const
{
	return _clientMaxBodySize;
}

const std::map<int, std::string> &Server::getStatusPages() const
{
	return _statusPages;
}

const std::map<std::string, Location> &Server::getLocations() const
{
	return _locations;
}

const std::string Server::getStatusPage(const int &status) const
{
	if (_statusPages.find(status) == _statusPages.end())
		return DefaultStatusMap::getStatusInfo(status);
	return _statusPages.at(status);
}

const Location Server::getLocation(const std::string &path) const
{
	if (_locations.find(path) == _locations.end())
		return Location();
	return _locations.at(path);
}

/*
** --------------------------------- SETTERS ---------------------------------
*/

void Server::setKeepAlive(const bool &keepAlive)
{
	_keepAlive = keepAlive;
}

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

void Server::setRoot(const std::string &root)
{
	_root = root;
}

void Server::setIndexes(const std::vector<std::string> &indexes)
{
	_indexes = indexes;
}

void Server::setAutoindex(const bool &autoindex)
{
	_autoindex = autoindex;
}

void Server::setClientMaxBodySize(const double &clientMaxBodySize)
{
	_clientMaxBodySize = clientMaxBodySize;
}

void Server::setStatusPages(const std::map<int, std::string> &statusPages)
{
	_statusPages = statusPages;
}

void Server::setLocations(const std::map<std::string, Location> &locations)
{
	_locations = locations;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void Server::addIndex(const std::string &index)
{
	if (std::find(_indexes.begin(), _indexes.end(), index) == _indexes.end())
		_indexes.push_back(index);
	else
		Logger::log(Logger::WARNING, "Index " + index + " already exists in server " + _serverName + " ignoring duplicate");
}

void Server::addLocation(const Location &location)
{
	if (_locations.find(location.getPath()) == _locations.end())
		_locations[location.getPath()] = location;
	else
		Logger::log(Logger::WARNING, "Location " + location.getPath() + " already exists in server " + _serverName + " ignoring duplicate");
}

void Server::addStatusPage(const int &status, const std::string &path)
{
	if (_statusPages.find(status) == _statusPages.end())
		_statusPages[status] = path;
	else
	{
		std::stringstream ss;
		ss << status;
		Logger::log(Logger::WARNING, "Status page " + ss.str() + " already exists in server " + _serverName + " ignoring duplicate");
	}
}

/* ************************************************************************** */
