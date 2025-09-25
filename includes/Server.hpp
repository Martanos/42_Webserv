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
	// Identifier members
	std::string _serverName;
	std::string _host;
	unsigned short _port;

	// Main members
	std::string _root;
	std::vector<std::string> _indexes;
	bool _autoindex;
	double _clientMaxBodySize;
	std::map<int, std::string> _statusPages;
	std::map<std::string, Location> _locations;
	bool _keepAlive;

public:
	Server();
	Server(Server const &src);
	Server(const std::string &serverName, const std::string &host, const unsigned short &port,
		   const ServerConfig &serverConfig);
	~Server();

	Server &operator=(Server const &rhs);

	// Getters
	const std::string &getServerName() const;
	const std::string &getHost() const;
	const unsigned short &getPort() const;
	const std::string &getRoot() const;
	const std::vector<std::string> &getIndexes() const;
	const bool &getAutoindex() const;
	const double &getClientMaxBodySize() const;
	const std::map<int, std::string> &getStatusPages() const;
	const std::string getStatusPage(const int &status) const;
	const std::map<std::string, Location> &getLocations() const;
	const Location getLocation(const std::string &path) const;
	const bool &getKeepAlive() const;

	// Setters
	void setKeepAlive(const bool &keepAlive);
	void setServerName(const std::string &serverName);
	void setHost(const std::string &host);
	void setPort(const unsigned short &port);
	void setRoot(const std::string &root);
	void setIndexes(const std::vector<std::string> &indexes);
	void setAutoindex(const bool &autoindex);
	void setClientMaxBodySize(const double &clientMaxBodySize);
	void setStatusPages(const std::map<int, std::string> &statusPages);
	void setLocations(const std::map<std::string, Location> &locations);

	// Methods
	void addIndex(const std::string &index);
	void addLocation(const Location &location);
	void addStatusPage(const int &status, const std::string &path);
};

std::ostream &operator<<(std::ostream &o, Server const &i);

#endif /* ********************************************************** SERVER_H                                          \
		*/
