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

	// Identifier flags
	bool _hasServerNameDirective;
	bool _hasSocketDirective;

	// Directive members
	std::string _rootPath;
	bool _autoIndexValue;
	std::string _cgiPath;
	double _clientMaxBodySize;
	bool _keepAlive;
	std::pair<int, std::string> _redirect;
	TrieTree<std::string> _indexes;
	std::map<int, std::string> _statusPages;
	std::vector<std::string> _allowedMethods;

	// Directive flags
	bool _hasRootPathDirective;
	bool _hasAutoIndexDirective;
	bool _hasCgiPathDirective;
	bool _hasClientMaxBodySizeDirective;
	bool _hasKeepAliveDirective;
	bool _hasRedirectDirective;
	bool _hasIndexDirective;
	bool _hasStatusPageDirective;
	bool _hasAllowedMethodsDirective;

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

	/* Directives */

	// Directive flag investigators
	bool hasRootPathDirective() const;
	bool hasAutoIndexDirective() const;
	bool hasCgiPathDirective() const;
	bool hasClientMaxBodySizeDirective() const;
	bool hasKeepAliveDirective() const;
	bool hasRedirectDirective() const;
	bool hasIndexesDirective() const;
	bool hasStatusPagesDirective() const;
	bool hasAllowedMethodsDirective() const;

	// Directive investigators
	bool hasIndex(const std::string &index) const;
	bool hasStatusPage(int status) const;
	bool hasAllowedMethod(const std::string &allowedMethod) const;

	// Directive accessors
	const std::string &getRootPath() const;
	bool getAutoIndexValue() const;
	const std::string &getCgiPath() const;
	double getClientMaxBodySize() const;
	bool getKeepAliveValue() const;
	const std::pair<int, std::string> &getRedirect() const;
	const TrieTree<std::string> &getIndexes() const;
	const std::string &getStatusPath(int status) const;
	const std::map<int, std::string> &getStatusPaths() const;
	const std::vector<std::string> &getAllowedMethods() const;

	// Directive Mutators
	void setRootPath(const std::string &root);
	void setAutoIndex(bool autoIndex);
	void setCgiPath(const std::string &cgiPath);
	void setClientMaxBodySize(double clientMaxBodySize);
	void setKeepAlive(bool keepAlive);
	void setRedirect(const std::pair<int, std::string> &redirect);
	void insertIndex(const std::string &index);
	void setIndexes(const TrieTree<std::string> &indexes);
	void insertStatusPath(const std::vector<int> &codes, const std::string &path);
	void setStatusPaths(const std::map<int, std::string> &statusPages);
	void insertAllowedMethod(const std::string &allowedMethod);
	void setAllowedMethods(const std::vector<std::string> &allowedMethods);

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
	bool isModified() const;
	void reset();
};

std::ostream &operator<<(std::ostream &o, Server const &i);

#endif /* ********************************************************** SERVER_H                                          \
		*/
