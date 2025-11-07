#include "../../includes/Core/Location.hpp"
#include <algorithm>
#include <iostream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Location::Location(const std::string &path)
{
	// Identifier members
	_path = path;
	_root = std::string();
	_allowedMethods = std::vector<std::string>();
	_statusPages = std::map<int, std::string>();
	_redirect = std::pair<int, std::string>();
	_indexes = TrieTree<std::string>();
	_cgiPath = std::string();
	_clientMaxBodySize = -1.0;
	_cgiParams = std::map<std::string, std::string>();
	_hasAutoIndex = false;

	// Flags
	_modified = false;
}

Location::Location(const Location &src)
{
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
		_hasAutoIndex = rhs._hasAutoIndex;
		_autoIndexValue = rhs._autoIndexValue;
		_indexes = rhs._indexes;
		_cgiPath = rhs._cgiPath;
		_clientMaxBodySize = rhs._clientMaxBodySize;
		_cgiParams = rhs._cgiParams;
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
	o << "Redirect: " << i.getRedirect().first << " " << i.getRedirect().second << std::endl;
	o << "Status pages: ";
	for (std::map<int, std::string>::const_iterator it = i.getStatusPages().begin(); it != i.getStatusPages().end();
		 ++it)
		o << it->first << ": " << it->second << " ";
	o << std::endl;
	o << "AutoIndex: " << i.hasAutoIndex() << std::endl;
	o << "Indexes: ";
	for (TrieTree<std::string>::const_iterator it = i.getIndexes().begin(); it != i.getIndexes().end(); ++it)
		o << *it << " ";
	o << std::endl;
	o << "CgiPath: " << i.getCgiPath() << std::endl;
	o << "--------------------------------" << std::endl;
}

/*
** --------------------------------- INVESTIGATORS ---------------------------------
*/

bool Location::hasAllowedMethod(const std::string &allowedMethod) const
{
	return std::find(_allowedMethods.begin(), _allowedMethods.end(), allowedMethod) != _allowedMethods.end();
}

bool Location::hasStatusPage(const int &status) const
{
	return _statusPages.find(status) != _statusPages.end();
}

bool Location::hasRedirect() const
{
	return _redirect.first != 0 && !_redirect.second.empty();
}

bool Location::hasAutoIndex() const
{
	return _hasAutoIndex;
}

bool Location::isAutoIndex() const
{
	return _autoIndexValue;
}
bool Location::hasIndexes() const
{
	return _indexes.size() > 0;
}

bool Location::hasIndex(const std::string &index) const
{
	return _indexes.find(index) != NULL;
}
bool Location::hasCgiPath() const
{
	return !_cgiPath.empty();
}

bool Location::hasClientMaxBodySize() const
{
	return _clientMaxBodySize >= 0.0;
}

bool Location::hasCgiParams() const
{
	return !_cgiParams.empty();
}

bool Location::hasModified() const
{
	return _modified;
}

bool Location::hasRoot() const
{
	return !_root.empty();
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

const std::pair<int, std::string> &Location::getRedirect() const
{
	return _redirect;
}

const std::map<int, std::string> &Location::getStatusPages() const
{
	return _statusPages;
}

const TrieTree<std::string> &Location::getIndexes() const
{
	return _indexes;
}

const std::string &Location::getCgiPath() const
{
	return _cgiPath;
}

double Location::getClientMaxBodySize() const
{
	return _clientMaxBodySize;
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
	_path = path;
}

void Location::setRoot(const std::string &root)
{
	_root = root;
	_modified = true;
}

void Location::insertAllowedMethod(const std::string &allowedMethod)
{
	_allowedMethods.push_back(allowedMethod);
	_modified = true;
}

void Location::insertStatusPage(const std::vector<int> &codes, const std::string &path)
{
	for (std::vector<int>::const_iterator it = codes.begin(); it != codes.end(); ++it)
	{
		_statusPages.insert(std::make_pair(*it, path));
	}
	_modified = true;
}

void Location::setRedirect(const std::pair<int, std::string> &redirect)
{
	_redirect = redirect;
	_modified = true;
}

void Location::setAutoIndex(const bool &autoIndex)
{
	_autoIndexValue = autoIndex;
	_hasAutoIndex = true;
	_modified = true;
}

void Location::insertIndex(const std::string &index)
{
	_indexes.insert(index, index);
	_modified = true;
}

void Location::setCgiPath(const std::string &cgiPath)
{
	_cgiPath = cgiPath;
	_modified = true;
}

void Location::setClientMaxBodySize(double size)
{
	_clientMaxBodySize = size;
	_modified = true;
}

void Location::setCgiParam(const std::string &key, const std::string &value)
{
	_cgiParams[key] = value;
	_modified = true;
}

/* ************************************************************************** */
