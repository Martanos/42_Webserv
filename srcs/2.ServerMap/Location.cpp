#include "../../includes/Core/Location.hpp"
#include <algorithm>
#include <iostream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Location::Location()
{
	_path = std::string();
	_root = std::string();
	_allowedMethods = std::vector<std::string>();
	_redirect = std::string();
	_autoIndex = false;
	_indexes = TrieTree<std::string>();
	_cgiPath = std::string();
	_pathSet = false;
	_rootSet = false;
	_autoIndexSet = false;
	_cgiPathSet = false;
	_redirectSet = false;
	_modified = false;
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
		_pathSet = rhs._pathSet;
		_rootSet = rhs._rootSet;
		_autoIndexSet = rhs._autoIndexSet;
		_cgiPathSet = rhs._cgiPathSet;
		_redirectSet = rhs._redirectSet;
		_modified = rhs._modified;
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
	o << "AutoIndex: " << i.isAutoIndex() << std::endl;
	o << "Indexes: ";
	for (TrieTree<std::string>::const_iterator it = i.getIndexes().begin(); it != i.getIndexes().end(); ++it)
		o << *it << " ";
	o << std::endl;
	o << "CgiPath: " << i.getCgiPath() << std::endl;
	o << "--------------------------------" << std::endl;
}

/*
** --------------------------------- ACCESSORS ---------------------------------
*/

const std::string &Location::getPath() const
{
	if (_path.empty())
		throw std::runtime_error("Path not set for location");
	return _path;
}

const std::string &Location::getRoot() const
{
	if (_root.empty())
		throw std::runtime_error("Root not set for location " + _path);
	return _root;
}

const std::vector<std::string> &Location::getAllowedMethods() const
{
	if (_allowedMethods.size() == 0)
		throw std::runtime_error("Allowed methods not set for location " + _path);
	return _allowedMethods;
}

bool Location::hasAllowedMethod(const std::string &allowedMethod) const
{
	return std::find(_allowedMethods.begin(), _allowedMethods.end(), allowedMethod) != _allowedMethods.end();
}

const std::string &Location::getRedirect() const
{
	if (_redirect.empty())
		throw std::runtime_error("Redirect not set for location " + _path);
	return _redirect;
}

bool Location::isAutoIndex() const
{
	return _autoIndex;
}

const TrieTree<std::string> &Location::getIndexes() const
{
	if (_indexes.size() == 0)
		throw std::runtime_error("Indexes not set for location " + _path);
	return _indexes;
}

const std::string &Location::getCgiPath() const
{
	if (_cgiPath.empty())
		throw std::runtime_error("Cgi path not set for location " + _path);
	return _cgiPath;
}

/*
** --------------------------------- Mutators ---------------------------------
*/

void Location::setPath(const std::string &path)
{
	_modified = true;
	if (_pathSet)
		throw std::runtime_error("Path already set for location " + _path);
	_path = path;
	_pathSet = true;
}

void Location::setRoot(const std::string &root)
{
	_modified = true;
	if (_rootSet)
		throw std::runtime_error("Root already set for location " + _root);
	_root = root;
	_rootSet = true;
}

void Location::insertAllowedMethod(const std::string &allowedMethod)
{
	_modified = true;
	if (!hasAllowedMethod(allowedMethod))
		_allowedMethods.push_back(allowedMethod);
}

void Location::setRedirect(const std::string &redirect)
{
	_modified = true;
	if (_redirect.empty())
		_redirect = redirect;
	else
		throw std::runtime_error("Redirect already set for location " + _redirect);
	_redirectSet = true;
}

void Location::setAutoIndex(const bool &autoIndex)
{
	_modified = true;
	if (_autoIndexSet)
		throw std::runtime_error("Auto index already set for location " + _path);
	_autoIndex = autoIndex;
	_autoIndexSet = true;
}

void Location::insertIndex(const std::string &index)
{
	_modified = true;
	_indexes.insert(index, index);
}

void Location::setCgiPath(const std::string &cgiPath)
{
	_modified = true;
	if (_cgiPathSet)
		throw std::runtime_error("Cgi path already set for location " + _cgiPath);
	_cgiPath = cgiPath;
	_cgiPathSet = true;
}

/* ************************************************************************** */
