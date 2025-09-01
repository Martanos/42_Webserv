#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "Logger.hpp"

//  Location object contains all required information for location
//  Param validation is done during parser stage, not here
//  Exists as reference to what the location block should contain
//  Contains methods relevant to server operations
class Location
{
private:
	std::string _path;
	std::string _root;
	std::vector<std::string> _allowedMethods;
	std::string _redirect;
	bool _autoIndex;
	std::string _index;
	std::string _cgiPath;
	std::map<std::string, std::string> _cgiParams;
	std::string _uploadPath;

public:
	Location();
	Location(Location const &src);
	~Location();

	Location &operator=(Location const &rhs);

	// Getters
	const std::string &getPath() const;
	const std::string &getRoot() const;
	const std::vector<std::string> &getAllowedMethods() const;
	const std::string &getRedirect() const;
	const bool &getAutoIndex() const;
	const std::string &getIndex() const;
	const std::string &getCgiPath() const;
	const std::map<std::string, std::string> &getCgiParams() const;
	const std::string &getUploadPath() const;

	// Setters
	void setPath(const std::string &path);
	void setRoot(const std::string &root);
	void setAllowedMethods(const std::vector<std::string> &allowedMethods);
	void setRedirect(const std::string &redirect);
	void setAutoIndex(const bool &autoIndex);
	void setIndex(const std::string &index);
	void setCgiPath(const std::string &cgiPath);
	void setCgiParams(const std::map<std::string, std::string> &cgiParams);
	void setUploadPath(const std::string &uploadPath);

	// Methods
	void addAllowedMethod(const std::string &allowedMethod);
	void addCgiParam(const std::string &cgiParam, const std::string &value);
};

std::ostream &operator<<(std::ostream &o, Location const &i);

#endif /* ******************************************************** LOCATION_H */
