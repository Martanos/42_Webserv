#ifndef SERVER_HPP
#define SERVER_HPP

#include "../../includes/Config/Directives.hpp"
#include "../../includes/Config/Location.hpp"
#include "../../includes/Containers/TrieTree.hpp"
#include "../../includes/Wrappers/SocketAddress.hpp"
#include <iostream>
#include <string>
#include <vector>

// Server configuration object
class Server : public Directives
{
private:
	// Identifier members
	TrieTree<std::string> _serverNames;
	std::vector<SocketAddress> _sockets;

	// Identifier flags
	bool _hasServerNameDirective;
	bool _hasSocketDirective;

	// Location blocks
	TrieTree<Location> _locations;

	// Location flags
	bool _hasLocationBlocks;

public:
	Server();
	Server(Server const &src);
	~Server();
	Server &operator=(Server const &rhs);

	/* Identifiers*/

	// Identifier flag investigators
	bool hasServerNameDirective() const;
	bool hasSocketDirective() const;

	// Identifier investigators
	bool hasServerName(const std::string &serverName) const;
	bool hasSocketAddress(const SocketAddress &socketAddress) const;

	// Identifier accessors
	const TrieTree<std::string> &getServerNames() const;
	const std::vector<SocketAddress> &getSocketAddresses() const;

	// Identifier mutators
	void insertServerName(const std::string &serverName);
	void insertSocketAddress(const SocketAddress &socketAddress);

	/* Locations */

	// Location flag investigators
	bool hasLocationBlocks() const;

	// Location investigators
	bool hasLocation(const std::string &path) const;
	bool hasLongestPrefixLocation(const std::string &path) const;

	// Location accessors
	const TrieTree<Location> &getLocations() const;
	TrieTree<Location> &getLocations();
	const Location *getLocation(const std::string &path) const;

	// Location Mutators
	void insertLocation(const Location &location);

	// Server Utils
	bool wasModified() const;
	void reset();
};

std::ostream &operator<<(std::ostream &o, Server const &i);

#endif /* ********************************************************** SERVER_H                                          \
		*/
