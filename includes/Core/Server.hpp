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
	bool _hasAutoIndex;
	bool _autoIndexValue;
	double _clientMaxBodySize;
	std::map<int, std::string> _statusPages;
	TrieTree<Location> _locations;
	bool _keepAlive;

	// Flags
	bool _modified;

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
	bool hasAutoIndex() const; // Line exists
	bool isAutoIndex() const;  // Line value
	bool isKeepAlive() const;
	bool isModified() const;
	bool hasRootPath() const;

	// Accessors
	const TrieTree<std::string> &getServerNames() const;
	const std::vector<SocketAddress> &getSocketAddresses() const;
	const std::string &getRootPath() const;
	const TrieTree<std::string> &getIndexes() const;
	double getClientMaxBodySize() const;
	const std::string &getStatusPath(int status) const;
	const std::map<int, std::string> &getStatusPages() const;
	const TrieTree<Location> &getLocations() const;
	const Location *getLocation(const std::string &path) const;

	// Mutators
	void insertServerName(const std::string &serverName);
	void insertSocketAddress(const SocketAddress &socketAddress);
	void insertIndex(const std::string &index);
	void insertLocation(const Location &location);
	void insertStatusPage(const std::string &path, const std::vector<int> &codes);
	void setKeepAlive(const bool &keepAlive);
	void setClientMaxBodySize(const double &clientMaxBodySize);
	void setRoot(const std::string &root);
	void setAutoindex(const bool &autoindex);

	void reset();
};

std::ostream &operator<<(std::ostream &o, Server const &i);

#endif /* ********************************************************** SERVER_H                                          \
		*/
