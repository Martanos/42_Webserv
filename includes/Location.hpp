#ifndef LOCATION_HPP
#define LOCATION_HPP

#include "TrieTree.hpp"
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
	std::string _path;
	bool _pathSet;
	std::string _root;
	bool _rootSet;
	std::vector<std::string> _allowedMethods;
	TrieTree<std::string> _indexes;
	bool _autoIndex;
	bool _autoIndexSet;
	std::string _redirect;
	bool _redirectSet;
	std::string _cgiPath;
	bool _cgiPathSet;
	std::map<std::string, std::string> _cgiParams;

public:
	Location();
	Location(Location const &src);
	~Location();
	Location &operator=(Location const &rhs);

	// Accessors
	const std::string &getPath() const;
	const std::string &getRoot() const;
	const std::string &getAllowedMethod(const std::string &allowedMethod) const;
	const std::vector<std::string> &getAllowedMethods() const;
	const std::string &getRedirect() const;
	const bool &getAutoIndex() const;
	const std::string *getIndex(const std::string &index) const;
	const TrieTree<std::string> &getIndexes() const;
	const std::string &getCgiPath() const;
	const std::map<std::string, std::string> &getCgiParams() const;
	const bool getPathSet() const;
	const bool getRootSet() const;
	const bool getAutoIndexSet() const;
	const bool getCgiPathSet() const;
	const bool getRedirectSet() const;

	// Mutators
	void setPath(const std::string &path);
	void setRoot(const std::string &root);
	void insertIndex(const std::string &index);
	void insertAllowedMethod(const std::string &allowedMethod);
	void setRedirect(const std::string &redirect);
	void setAutoIndex(const bool &autoIndex);
	void setCgiPath(const std::string &cgiPath);
	void insertCgiParam(const std::string &cgiParam, const std::string &value);
};

void operator<<(std::ostream &o, Location const &i);

#endif /* ******************************************************** LOCATION_H                                          \
		*/
