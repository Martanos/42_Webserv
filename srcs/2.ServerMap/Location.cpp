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
		_autoIndex = rhs._autoIndex;
		_indexes = rhs._indexes;
		_cgiPath = rhs._cgiPath;
		_modified = rhs._modified;
	}
	return *this;
}

std::ostream &operator<<(std::ostream &o, Location const &i)
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
	return o;
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
	return _redirect.first != 0 && _redirect.second.empty();
}

bool Location::hasAutoIndex() const
{
	return _autoIndex;
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
	return _cgiPath.empty();
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
	_autoIndex = autoIndex;
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

/* ************************************************************************** */
