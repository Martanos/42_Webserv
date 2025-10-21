#ifndef SERVER_HPP
#define SERVER_HPP

#include "../../includes/ConfigParser/ServerConfig.hpp"
#include "../../includes/Core/Location.hpp"
#include "../../includes/Global/DefaultStatusMap.hpp"
#include "../../includes/Global/IPAddressParser.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/Wrapper/Socket.hpp"
#include "../../includes/Wrapper/TrieTree.hpp"
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
	std::vector<Socket> _sockets;

	// Main members
	std::string _root;
	TrieTree<std::string> _indexes;
	bool _autoindex;
	double _clientMaxUriSize;
	double _clientMaxHeadersSize;
	double _clientMaxBodySize;
	std::vector<std::pair<int, std::vector<std::string> > > _statusPages;
	TrieTree<Location> _locations;
	bool _keepAlive;

	// Flags
	bool _rootSet;
	bool _autoindexSet;
	bool _clientMaxUriSizeSet;
	bool _clientMaxHeadersSizeSet;
	bool _clientMaxBodySizeSet;
	bool _keepAliveSet;
	bool _modified;

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
	void insertSocket(const Socket &socket);
	void insertIndex(const std::string &index);
	void insertLocation(const Location &location);
	void insertStatusPage(const std::string &path, const std::vector<int> &codes);
	void setKeepAlive(const bool &keepAlive);
	void setClientMaxUriSize(const double &clientMaxUriSize);
	void setClientMaxHeadersSize(const double &clientMaxHeadersSize);
	void setClientMaxBodySize(const double &clientMaxBodySize);
	void setRoot(const std::string &root);
	void setAutoindex(const bool &autoindex);

	bool isModified() const;
	void reset();
};

std::ostream &operator<<(std::ostream &o, Server const &i);

#endif /* ********************************************************** SERVER_H                                          \
		*/
