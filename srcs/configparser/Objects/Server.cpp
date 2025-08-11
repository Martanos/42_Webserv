#include "Server.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Server::Server()
{
	_serverName = "";
	_host_port = std::make_pair("localhost", 80);
	_root = "www/";
	_indexes = std::vector<std::string>();
	_autoindex = false;
	_clientMaxBodySize = 1000000;
	_statusPages = std::map<int, std::vector<std::string> >();
	_locations = std::map<std::string, Location>();
}

Server::Server(const Server &src)
{
	if (this != &src)
	{
		_serverName = src._serverName;
		_host_port = src._host_port;
		_root = src._root;
		_indexes = src._indexes;
		_autoindex = src._autoindex;
		_clientMaxBodySize = src._clientMaxBodySize;
		_statusPages = src._statusPages;
		_locations = src._locations;
	}
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
		_host_port = rhs._host_port;
		_root = rhs._root;
		_indexes = rhs._indexes;
		_autoindex = rhs._autoindex;
		_clientMaxBodySize = rhs._clientMaxBodySize;
		_statusPages = rhs._statusPages;
		_locations = rhs._locations;
	}
	return *this;
}

std::ostream &operator<<(std::ostream &o, Server const &i)
{
	o << "--------------------------------" << std::endl;
	o << "Server name: " << i.getServerName() << std::endl;
	o << "Host/port: " << i.getHost_port().first << ":" << i.getHost_port().second << std::endl;
	o << "Root: " << i.getRoot() << std::endl;
	o << "Indexes: ";
	for (std::vector<std::string>::const_iterator it = i.getIndexes().begin(); it != i.getIndexes().end(); ++it)
		o << *it << " ";
	o << std::endl;
	o << "Autoindex: " << (i.getAutoindex() ? "true" : "false") << std::endl;
	o << "Client max body size: " << i.getClientMaxBodySize() << std::endl;
	o << "Status pages: ";
	for (std::map<int, std::vector<std::string> >::const_iterator it = i.getStatusPages().begin(); it != i.getStatusPages().end(); ++it)
		o << it->first << ": " << it->second.at(0) << " ";
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

const std::string &Server::getServerName() const
{
	return _serverName;
}

const std::pair<std::string, unsigned short> &Server::getHost_port() const
{
	return _host_port;
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

const std::map<int, std::vector<std::string> > &Server::getStatusPages() const
{
	return _statusPages;
}

const std::map<std::string, Location> &Server::getLocations() const
{
	return _locations;
}

const std::vector<std::string> &Server::getStatusPage(int &status) const
{
	if (_statusPages.find(status) == _statusPages.end())
		return DefaultStatusMap::getStatusInfo(status);
	return _statusPages.at(status);
}

const Location &Server::getLocation(const std::string &path) const
{
	if (_locations.find(path) == _locations.end())
		return Location();
	return _locations.at(path);
}

/*
** --------------------------------- SETTERS ---------------------------------
*/

void Server::setServerName(const std::string &serverName)
{
	_serverName = serverName;
}

void Server::setHost_port(const std::pair<std::string, unsigned short> &host_port)
{
	_host_port = host_port;
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

void Server::setStatusPages(const std::map<int, std::vector<std::string> > &statusPages)
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

void Server::addStatusPage(const int &status, const std::vector<std::string> &paths)
{
	if (_statusPages.find(status) == _statusPages.end())
		_statusPages[status] = paths;
	else
	{
		std::stringstream ss;
		ss << status;
		Logger::log(Logger::WARNING, "Status page " + ss.str() + " already exists in server " + _serverName + " ignoring duplicate");
	}
}

/* ************************************************************************** */
