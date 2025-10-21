#ifndef LOCATION_HPP
#define LOCATION_HPP

#include "../../includes/Wrapper/TrieTree.hpp"
#include <map>
#include <string>
#include <vector>

//  Location object contains all required information for location
//  Param validation is done during parser stage, not here
//  Exists as reference to what the location block should contain
//  Contains methods relevant to server operations
class Location
{
private:
	// Identifier members
	std::string _path;
	std::string _root;
	std::vector<std::string> _allowedMethods;
	TrieTree<std::string> _indexes;
	bool _autoIndex;
	std::pair<int, std::string> _redirect;
	std::string _cgiPath;

	// Flags
	bool _pathSet;
	bool _rootSet;
	bool _autoIndexSet;
	bool _cgiPathSet;
	bool _redirectSet;
	bool _modified;

public:
	Location();
	Location(Location const &src);
	~Location();
	Location &operator=(Location const &rhs);

	// Accessors
	const std::string &getPath() const;
	const std::string &getRoot() const;
	bool hasAllowedMethod(const std::string &allowedMethod) const;
	const std::vector<std::string> &getAllowedMethods() const;
	const std::pair<int, std::string> &getRedirect() const;
	bool isAutoIndex() const;
	const TrieTree<std::string> &getIndexes() const;
	const std::string &getCgiPath() const;
	const bool getPathSet() const;
	const bool getRootSet() const;
	const bool getAutoIndexSet() const;
	const bool getCgiPathSet() const;
	const bool getRedirectSet() const;
	const bool isModified() const;

	// Mutators
	void setPath(const std::string &path);
	void setRoot(const std::string &root);
	void insertIndex(const std::string &index);
	void insertAllowedMethod(const std::string &allowedMethod);
	void setRedirect(const std::pair<int, std::string> &redirect);
	void setAutoIndex(const bool &autoIndex);
	void setCgiPath(const std::string &cgiPath);
};

void operator<<(std::ostream &o, Location const &i);

#endif /* ******************************************************** LOCATION_H                                          \
		*/
