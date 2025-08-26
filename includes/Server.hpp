#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "Logger.hpp"
#include "Location.hpp"
#include "ServerConfig.hpp"
#include "DefaultStatusMap.hpp"

// Main server object contains all required information for server
// Param validation is done during parser stage, not here
// Exists as reference to what the server block should contain
// Contains methods relevant to server operations
class Server
{
private:
	// Identifier members
	std::string _serverName;
	std::pair<std::string, unsigned short> _host_port;

	// Main members
	std::string _root;
	std::vector<std::string> _indexes;
	bool _autoindex;
	double _clientMaxBodySize;
	std::map<int, std::vector<std::string> > _statusPages;
	std::map<std::string, Location> _locations;
	bool _keepAlive;

public:
	Server();
	Server(Server const &src);
	Server(const std::string &serverName, const std::pair<std::string, unsigned short> &host_port, const ServerConfig &serverConfig);
	~Server();

	Server &operator=(Server const &rhs);

	// Getters
	const std::string &getServerName() const;
	const std::pair<std::string, unsigned short> &getHost_port() const;
	const std::string &getRoot() const;
	const std::vector<std::string> &getIndexes() const;
	const bool &getAutoindex() const;
	const double &getClientMaxBodySize() const;
	const std::map<int, std::vector<std::string> > &getStatusPages() const;
	const std::vector<std::string> &getStatusPage(int &status) const;
	const std::map<std::string, Location> &getLocations() const;
	const Location &getLocation(const std::string &path) const;

	// Setters
	void setServerName(const std::string &serverName);
	void setHost_port(const std::pair<std::string, unsigned short> &host_port);
	void setRoot(const std::string &root);
	void setIndexes(const std::vector<std::string> &indexes);
	void setAutoindex(const bool &autoindex);
	void setClientMaxBodySize(const double &clientMaxBodySize);
	void setStatusPages(const std::map<int, std::vector<std::string> > &statusPages);
	void setLocations(const std::map<std::string, Location> &locations);

	// Methods
	void addIndex(const std::string &index);
	void addLocation(const Location &location);
	void addStatusPage(const int &status, const std::vector<std::string> &paths);
};

std::ostream &operator<<(std::ostream &o, Server const &i);

#endif /* ********************************************************** SERVER_H */
