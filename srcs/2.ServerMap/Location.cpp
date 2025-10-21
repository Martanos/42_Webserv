#include "../../includes/Core/Location.hpp"
#include "../../includes/Logger.hpp"
#include <algorithm>
#include <iostream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Location::Location()
{
	_path = "";
	_root = "";
	_allowedMethods = std::vector<std::string>();
	_redirect = "";
	_autoIndex = false;
	_indexes = TrieTree<std::string>();
	_cgiPath = "";
	_cgiParams = std::map<std::string, std::string>();
	_pathSet = false;
	_rootSet = false;
	_autoIndexSet = false;
	_cgiPathSet = false;
	_redirectSet = false;
}

Location::Location(const Location &src)
{
	if (this != &src)
		*this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Location::~Location()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

Location &Location::operator=(Location const &rhs)
{
	if (this != &rhs)
	{
		_path = rhs._path;
		_root = rhs._root;
		_allowedMethods = rhs._allowedMethods;
		_redirect = rhs._redirect;
		_autoIndex = rhs._autoIndex;
		_indexes = rhs._indexes;
		_cgiPath = rhs._cgiPath;
		_cgiParams = rhs._cgiParams;
		_pathSet = rhs._pathSet;
		_rootSet = rhs._rootSet;
		_autoIndexSet = rhs._autoIndexSet;
		_cgiPathSet = rhs._cgiPathSet;
		_redirectSet = rhs._redirectSet;
	}
	return *this;
}

void operator<<(std::ostream &o, Location const &i)
{
	o << "--------------------------------" << std::endl;
	o << "Path: " << i.getPath() << std::endl;
	o << "Root: " << i.getRoot() << std::endl;
	o << "Allowed Methods: ";
	for (std::vector<std::string>::const_iterator it = i.getAllowedMethods().begin(); it != i.getAllowedMethods().end();
		 ++it)
		o << *it << " ";
	o << std::endl;
	o << "Redirect: " << i.getRedirect() << std::endl;
	o << "AutoIndex: " << i.getAutoIndex() << std::endl;
	o << "Indexes: ";
	for (TrieTree<std::string>::const_iterator it = i.getIndexes().begin(); it != i.getIndexes().end(); ++it)
		o << *it << " ";
	o << std::endl;
	o << "CgiPath: " << i.getCgiPath() << std::endl;
	o << "CgiParams: ";
	for (std::map<std::string, std::string>::const_iterator it = i.getCgiParams().begin(); it != i.getCgiParams().end();
		 ++it)
		o << it->first << "=" << it->second << " ";
	o << std::endl;
	o << "--------------------------------" << std::endl;
}

/*
** --------------------------------- ACCESSORS ---------------------------------
*/

const std::string &Location::getPath() const
{
	return _path;
}

const std::string &Location::getRoot() const
{
	return _root;
}

const std::vector<std::string> &Location::getAllowedMethods() const
{
	return _allowedMethods;
}

const std::string &Location::getAllowedMethod(const std::string &allowedMethod) const
{
	return *std::find(_allowedMethods.begin(), _allowedMethods.end(), allowedMethod);
}

const std::string &Location::getRedirect() const
{
	return _redirect;
}

const bool &Location::getAutoIndex() const
{
	return _autoIndex;
}

const std::string *Location::getIndex(const std::string &index) const
{
	return _indexes.find(index);
}

const TrieTree<std::string> &Location::getIndexes() const
{
	return _indexes;
}

const std::string &Location::getCgiPath() const
{
	return _cgiPath;
}

const std::map<std::string, std::string> &Location::getCgiParams() const
{
	return _cgiParams;
}

/*
** --------------------------------- Mutators ---------------------------------
*/

void Location::setPath(const std::string &path)
{
	if (_pathSet)
		throw std::runtime_error("Path already set for location " + _path);
	_pathSet = true;
	_path = path;
}

void Location::setRoot(const std::string &root)
{
	if (_rootSet)
		throw std::runtime_error("Root already set for location " + _root);
	_rootSet = true;
	_root = root;
}

void Location::insertAllowedMethod(const std::string &allowedMethod)
{
	if (std::find(_allowedMethods.begin(), _allowedMethods.end(), allowedMethod) == _allowedMethods.end())
		_allowedMethods.push_back(allowedMethod);
	else
		throw std::runtime_error("Allowed method " + allowedMethod + " already exists in location " + _path);
}

void Location::setRedirect(const std::string &redirect)
{
	if (_redirectSet)
		throw std::runtime_error("Redirect already set for location " + _redirect);
	_redirectSet = true;
	_redirect = redirect;
}

void Location::setAutoIndex(const bool &autoIndex)
{
	if (_autoIndexSet)
		throw std::runtime_error("Auto index already set for location " + _path);
	_autoIndexSet = true;
	_autoIndex = autoIndex;
}

void Location::insertIndex(const std::string &index)
{
	if (_indexes.find(index) == NULL)
		_indexes.insert(index, index);
	else
		throw std::runtime_error("Index " + index + " already exists in location " + _path);
}

void Location::setCgiPath(const std::string &cgiPath)
{
	if (_cgiPathSet)
		throw std::runtime_error("Cgi path already set for location " + _cgiPath);
	_cgiPathSet = true;
	_cgiPath = cgiPath;
}

void Location::insertCgiParam(const std::string &cgiParam, const std::string &value)
{
	if (_cgiParams.find(cgiParam) == _cgiParams.end())
		_cgiParams[cgiParam] = value;
	else
		throw std::runtime_error("Cgi param " + cgiParam + " already exists in location " + _path);
}

/* ************************************************************************** */
