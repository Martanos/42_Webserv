#ifndef SERVER_HPP
#define SERVER_HPP

#include "../../includes/Core/Location.hpp"
#include "../../includes/Wrapper/SocketAddress.hpp"
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
	std::vector<SocketAddress> _sockets;

	// Main members
	std::string _rootPath;
	TrieTree<std::string> _indexes;
	bool _autoIndex;
	double _clientMaxUriSize;
	double _clientMaxHeadersSize;
	double _clientMaxBodySize;
	std::map<int, std::string> _statusPages;
	TrieTree<Location> _locations;
	bool _keepAlive;

	// Flags
	bool _modified;

	// Configuration pointer
	void *_config;

public:
	Server();
	Server(Server const &src);
	~Server();
	Server &operator=(Server const &rhs);

	// Investigators
	bool hasServerName(const std::string &serverName) const;
	bool hasSocketAddress(const SocketAddress &socketAddress) const;
	bool hasIndex(const std::string &index) const;
	bool hasLocation(const std::string &path) const;
	bool hasStatusPage(int status) const;
	bool isAutoIndex() const;
	bool isKeepAlive() const;
	bool isModified() const;

	// Accessors
	const TrieTree<std::string> &getServerNames() const;
	const std::vector<SocketAddress> &getSocketAddresses() const;
	const std::string &getRootPath() const;
	const TrieTree<std::string> &getIndexes() const;
	double getClientMaxUriSize() const;
	double getClientMaxHeadersSize() const;
	double getClientMaxBodySize() const;
	const std::string &getStatusPath(int status) const;
	const std::map<int, std::string> &getStatusPages() const;
	const TrieTree<Location> &getLocations() const;
	const Location *getLocation(const std::string &path) const;

	// Additional accessors for compatibility
	const std::string getServerName() const;
	const std::string getHost() const;
	const std::string getRoot() const;
	unsigned short getPort() const;

	// Mutators
	void insertServerName(const std::string &serverName);
	void insertSocketAddress(const SocketAddress &socketAddress);
	void insertIndex(const std::string &index);
	void insertLocation(const Location &location);
	void insertStatusPage(const std::string &path, const std::vector<int> &codes);
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
