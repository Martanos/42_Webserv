#include "../../includes/Server.hpp"
#include "../../includes/Constants.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Server::Server()
	: _config(NULL)
{
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
		_config = rhs._config;
		_locations = rhs._locations;
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
	for (std::map<int, std::string>::const_iterator it = i.getStatusPages().begin(); it != i.getStatusPages().end();
		 ++it)
		o << it->first << ": " << it->second << " ";
	o << std::endl;
	o << "Locations: ";
	for (std::map<std::string, Location>::const_iterator it = i.getLocations().begin(); it != i.getLocations().end();
		 ++it)
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

bool Server::getKeepAlive() const
{
	if (_config)
		return _config->getKeepAlive();
	return SERVER::DEFAULT_KEEP_ALIVE; // Need to define this constant
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
	if (_config)
		return _config->getRoot();
	static const std::string defaultRoot = SERVER::DEFAULT_ROOT;
	return defaultRoot;
}

const std::vector<std::string> &Server::getIndexes() const
{
	if (_config)
		return _config->getIndexes();
	static const std::vector<std::string> defaultIndexes(1, SERVER::DEFAULT_INDEX);
	return defaultIndexes;
}

bool Server::getAutoindex() const
{
	if (_config)
		return _config->getAutoindex();
	return SERVER::DEFAULT_AUTOINDEX;
}

double Server::getClientMaxBodySize() const
{
	if (_config)
		return _config->getClientMaxBodySize();
	return SERVER::DEFAULT_CLIENT_MAX_BODY_SIZE;
}

const std::map<int, std::string> &Server::getStatusPages() const
{
	if (_config)
		return _config->getStatusPages();
	static const std::map<int, std::string> emptyMap;
	return emptyMap;
}

const std::map<std::string, Location> &Server::getLocations() const
{
	return _locations;
}

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

/*
** --------------------------------- SETTERS ---------------------------------
*/

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
{
	_locations = locations;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void Server::addLocation(const Location &location)
{
	if (_locations.find(location.getPath()) == _locations.end())
		_locations[location.getPath()] = location;
	else
		Logger::log(Logger::WARNING, "Location " + location.getPath() + " already exists in server " + _serverName +
										 " ignoring duplicate");
}

/* ************************************************************************** */
