#ifndef SERVER_HPP
#define SERVER_HPP

#include "DefaultStatusMap.hpp"
#include "Location.hpp"
#include "Logger.hpp"
#include "ServerConfig.hpp"
#include "TrieTree.hpp"
#include <iostream>
#include <map>
#include <string>
#include <vector>

// Server configuration object
class Server
{
private:
	// Identifier members
	TrieTree<std::string> _serverNames;
	std::vector<std::pair<std::string, unsigned short> > _hosts_ports;

	// Main members
	std::string _root;
	bool _rootSet;
	TrieTree<std::string> _indexes;
	bool _autoindex;
	bool _autoindexSet;
	double _clientMaxUriSize;
	bool _clientMaxUriSizeSet;
	double _clientMaxHeadersSize;
	bool _clientMaxHeadersSizeSet;
	double _clientMaxBodySize;
	bool _clientMaxBodySizeSet;
	std::map<int, std::string> _statusPages;
	TrieTree<Location> _locations;
	bool _keepAlive;
	bool _keepAliveSet;

public:
	Server();
	Server(Server const &src);
	~Server();
	Server &operator=(Server const &rhs);

	// Accessors
	const std::string *getServerName(const std::string &serverName) const;
	const TrieTree<std::string> &getServerNames() const;
	const std::pair<std::string, unsigned short> *getHostPort(const std::string &host,
															  const unsigned short &port) const;
	const std::vector<std::pair<std::string, unsigned short> > &getHosts_ports() const;
	const std::string &getRoot() const;
	const std::string *getIndex(const std::string &index) const;
	const TrieTree<std::string> &getIndexes() const;
	const bool getAutoindex() const;
	const double &getClientMaxUriSize() const;
	const double &getClientMaxHeadersSize() const;
	const double &getClientMaxBodySize() const;
	const std::string *getStatusPage(const int &status) const;
	const std::map<int, std::string> &getStatusPages() const;
	const TrieTree<Location> &getLocations() const;
	const Location *getLocation(const std::string &path) const;
	const bool getKeepAlive() const;

	// Mutators
	void insertServerName(const std::string &serverName);
	void insertHostPort(const std::string &host, const unsigned short &port);
	void insertIndex(const std::string &index);
	void insertLocation(const Location &location);
	void insertStatusPage(const int &status, const std::string &path);
	void setKeepAlive(const bool &keepAlive);
	void setClientMaxUriSize(const double &clientMaxUriSize);
	void setClientMaxHeadersSize(const double &clientMaxHeadersSize);
	void setClientMaxBodySize(const double &clientMaxBodySize);
	void setRoot(const std::string &root);
	void setAutoindex(const bool &autoindex);

	void reset();
};

std::ostream &operator<<(std::ostream &o, Server const &i);

#endif /* ********************************************************** SERVER_H                                          \
		*/
