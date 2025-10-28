#ifndef SERVER_HPP
#define SERVER_HPP

#include "DefaultStatusMap.hpp"
#include "Location.hpp"
#include "Logger.hpp"
#include "ServerConfig.hpp"
#include <iostream>
#include <map>
#include <string>
#include <vector>

// Main server object contains all required information for server
// Param validation is done during parser stage, not here
// Exists as reference to what the server block should contain
// Contains methods relevant to server operations
class Server
{
private:
	// Server-specific data (expanded per server instance)
	std::string _serverName;
	std::string _host;
	unsigned short _port;

	// Reference to shared server config (not expanded per server instance)
	const ServerConfig* _config;

	// Locations (processed per server instance)
	std::map<std::string, Location> _locations;

public:
	Server();
	Server(Server const &src);
	Server(const std::string &serverName, const std::string &host, const unsigned short &port,
		   const ServerConfig* serverConfig);
	~Server();

	Server &operator=(Server const &rhs);

	// Getters
	const std::string &getServerName() const;
	const std::string &getHost() const;
	const unsigned short &getPort() const;
	const std::string &getRoot() const;
	const std::vector<std::string> &getIndexes() const;
	bool getAutoindex() const;
	double getClientMaxBodySize() const;
	const std::map<int, std::string> &getStatusPages() const;
	const std::string getStatusPage(const int &status) const;
	const std::map<std::string, Location> &getLocations() const;
	const Location getLocation(const std::string &path) const;
	bool getKeepAlive() const;

	// Setters (only for server-specific data)
	void setServerName(const std::string &serverName);
	void setHost(const std::string &host);
	void setPort(const unsigned short &port);
	void setLocations(const std::map<std::string, Location> &locations);

	// Methods
	void addLocation(const Location &location);
};

std::ostream &operator<<(std::ostream &o, Server const &i);

#endif /* ********************************************************** SERVER_H                                          \
		*/
