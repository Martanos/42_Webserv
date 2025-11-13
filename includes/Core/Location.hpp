#ifndef LOCATION_HPP
#define LOCATION_HPP

#include "../../includes/Wrapper/TrieTree.hpp"
#include <map>
#include <string>
#include <vector>

// Location configuration object
// Basically a smaller server block with path matching
class Location
{
private:
	// Identifier members
	std::string _locationPath;

	// Directive members
	std::string _rootPath;
	bool _autoIndexValue;
	std::string _cgiPath;
	double _clientMaxBodySize;
	bool _keepAliveValue;
	std::pair<int, std::string> _redirect;
	TrieTree<std::string> _indexes;
	std::map<int, std::string> _statusPages;
	std::vector<std::string> _allowedMethods;

	// Flags
	bool _hasRootPathDirective;
	bool _hasAutoIndexDirective;
	bool _hasCgiPathDirective;
	bool _hasClientMaxBodySizeDirective;
	bool _hasKeepAliveDirective;
	bool _hasRedirectDirective;
	bool _hasIndexDirective;
	bool _hasStatusPagesDirective;
	bool _hasAllowedMethodsDirective;

	// OCF ownership
	Location();

public:
	explicit Location(const std::string &path);
	Location(Location const &src);
	~Location();
	Location &operator=(Location const &rhs);

	/* Identifiers */

	// Identifier accessors
	const std::string &getLocationPath() const;

	/* Directives */

	// Directive flags
	bool hasRootPathDirective() const;
	bool hasAutoIndexDirective() const;
	bool hasCgiPathDirective() const;
	bool hasClientMaxBodySizeDirective() const;
	bool hasKeepAliveDirective() const;
	bool hasRedirectDirective() const;
	bool hasIndexDirective() const;
	bool hasStatusPagesDirective() const;
	bool hasAllowedMethodsDirective() const;

	// Directive Investigators
	bool hasIndex(const std::string &index) const;
	bool hasStatusPage(const int &status) const;
	bool hasAllowedMethod(const std::string &allowedMethod) const;

	// Directive Accessors
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

	// Mutators
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
	// Utility
	bool wasModified() const;
	void reset();
};

std::ostream &operator<<(std::ostream &o, Location const &i);

#endif /* ******************************************************** LOCATION_H                                          \
		*/
