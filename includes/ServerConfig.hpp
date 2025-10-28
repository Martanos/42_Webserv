#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include "Location.hpp"
#include "Logger.hpp"
#include <cstring>
#include <map>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

#include <vector>

#define DEFAULT_SERVER_NAME "default"

// Temp container to gather all config data
class ServerConfig
{
private:
	// Identifier directives
	std::vector<std::string> _serverNames;
	std::vector<std::pair<std::string, unsigned short> > _hosts_ports;

	// Main directives
	std::string _root;
	std::vector<std::string> _indexes;
	bool _autoindex;
	double _maxUriSize;
	double _maxHeaderSize;
	double _clientMaxBodySize;
	std::map<int, std::string> _statusPages;
	std::vector<Location> _locations;
	std::string _accessLog;
	std::string _errorLog;
	bool _keepAlive;

	// Private static utilities
	static bool try_validate_port_only(std::string token);
	static bool try_validate_host_only(std::string token);
	static std::pair<std::string, unsigned short> split_host_port(std::string token);

public:
	ServerConfig();
	~ServerConfig();
	ServerConfig(const ServerConfig &other);
	ServerConfig &operator=(const ServerConfig &other);

	// Getters
	const std::vector<std::string> &getServerNames() const;
	const std::vector<std::pair<std::string, unsigned short> > &getHosts_ports() const;
	const std::string &getRoot() const;
	const std::vector<std::string> &getIndexes() const;
	bool getAutoindex() const;
	double getMaxUriSize() const;
	double getMaxHeaderSize() const;
	double getClientMaxBodySize() const;
	const std::map<int, std::string> &getStatusPages() const;
	const std::vector<Location> &getLocations() const;
	const std::string &getAccessLog() const;
	const std::string &getErrorLog() const;
	bool getKeepAlive() const;

	// Setters
	// void setServerNames(const std::vector<std::string> &serverNames);
	// void setHosts_ports(const std::vector<std::pair<std::string, unsigned
	// short> > &hosts_ports); void setRoot(const std::string &root); void
	// setIndexes(const std::vector<std::string> &indexes); void
	// setAutoindex(bool autoindex); void setClientMaxBodySize(double size);
	// void setResponsePages(const std::map<int, std::string> &responsePages);
	// void setAccessLog(const std::string &accessLog);
	// void setErrorLog(const std::string &errorLog);
	// void setLocations(const std::vector<LocationConfig> &locations);

	// Parsing methods
	void addServerName(std::string line, double lineNumber);
	void addHosts_ports(std::string line, double lineNumber);
	void addIndexes(std::string line, double lineNumber);
	void addStatusPages(std::string line, double lineNumber);
	void addRoot(std::string line, double lineNumber);
	void addMaxUriSize(std::string line, double lineNumber);
	void addMaxHeaderSize(std::string line, double lineNumber);
	void addClientMaxBodySize(std::string line, double lineNumber);
	void addAutoindex(std::string line, double lineNumber);
	void addAccessLog(std::string line, double lineNumber);
	void addErrorLog(std::string line, double lineNumber);
	void addLocation(const Location &location, double lineNumber);
	void addKeepAlive(std::string line, double lineNumber);

	// Utils
	bool hasLocation(const Location &location) const;

	// Debugging methods
	void printConfig() const;
};

#endif
