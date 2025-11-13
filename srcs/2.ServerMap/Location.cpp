#include "../../includes/Core/Location.hpp"
#include <algorithm>
#include <iostream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Location::Location(const std::string &locationPath)
{
	// Identifier members
	_locationPath = locationPath;
	reset();
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
** --------------------------------- OVERLOADS ---------------------------------
*/

Location &Location::operator=(Location const &rhs)
{
	if (this != &rhs)
	{
		// Identifiers
		_locationPath = rhs._locationPath;

		// Directives
		_rootPath = rhs._rootPath;
		_autoIndexValue = rhs._autoIndexValue;
		_cgiPath = rhs._cgiPath;
		_clientMaxBodySize = rhs._clientMaxBodySize;
		_keepAliveValue = rhs._keepAliveValue;
		_redirect = rhs._redirect;
		_indexes = rhs._indexes;
		_statusPages = rhs._statusPages;
		_allowedMethods = rhs._allowedMethods;

		// Flags
		_hasRootPathDirective = rhs._hasRootPathDirective;
		_hasAutoIndexDirective = rhs._hasAutoIndexDirective;
		_hasCgiPathDirective = rhs._hasCgiPathDirective;
		_hasClientMaxBodySizeDirective = rhs._hasClientMaxBodySizeDirective;
		_hasKeepAliveDirective = rhs._hasKeepAliveDirective;
		_hasRedirectDirective = rhs._hasRedirectDirective;
		_hasIndexDirective = rhs._hasIndexDirective;
		_hasStatusPagesDirective = rhs._hasStatusPagesDirective;
		_hasAllowedMethodsDirective = rhs._hasAllowedMethodsDirective;
	}
	return *this;
}

std::ostream &operator<<(std::ostream &o, Location const &i)
{
	o << "--------------------------------" << std::endl;
	o << "Path: " << i.getLocationPath() << std::endl;
	if (i.hasRootPathDirective())
		o << "Root: " << i.getRootPath() << std::endl;
	if (i.hasAutoIndexDirective())
		o << "AutoIndex: " << (i.getAutoIndexValue() ? "on" : "off") << std::endl;
	if (i.hasCgiPathDirective())
		o << "CgiPath: " << i.getCgiPath() << std::endl;
	if (i.hasClientMaxBodySizeDirective())
		o << "Client Max Body Size: " << i.getClientMaxBodySize() << std::endl;
	if (i.hasAllowedMethodsDirective())
	{
		o << "Allowed Methods: ";
		const std::vector<std::string> &methods = i.getAllowedMethods();
		for (size_t idx = 0; idx < methods.size(); ++idx)
		{
			o << methods[idx];
			if (idx < methods.size() - 1)
				o << ", ";
		}
		o << std::endl;
	}
	if (i.hasIndexDirective())
	{
		o << "Indexes: " << std::endl;
		const std::vector<std::string> &indexes = i.getIndexes().getAllKeys();
		for (size_t idx = 0; idx < indexes.size(); ++idx)
		{
			o << "  " << indexes[idx] << std::endl;
		}
	}
	if (i.hasStatusPagesDirective())
	{
		o << "Status Pages: " << std::endl;
		const std::map<int, std::string> &statusPages = i.getStatusPaths();
		for (std::map<int, std::string>::const_iterator it = statusPages.begin(); it != statusPages.end(); ++it)
		{
			o << "  " << it->first << " -> " << it->second << std::endl;
		}
	}
	if (i.hasRedirectDirective())
	{
		const std::pair<int, std::string> &redirect = i.getRedirect();
		o << "Redirect: " << redirect.first << " -> " << redirect.second << std::endl;
	}
	o << "--------------------------------" << std::endl;
	return o;
}

/*
** --------------------------------- IDENTIFIER ACCESSORS ---------------------------------
*/

const std::string &Location::getLocationPath() const
{
	return _locationPath;
}

/*
** --------------------------------- DIRECTIVE FLAGS ---------------------------------
*/

bool Location::hasRootPathDirective() const
{
	return _hasRootPathDirective;
}
bool Location::hasAutoIndexDirective() const
{
	return _hasAutoIndexDirective;
}
bool Location::hasCgiPathDirective() const
{
	return _hasCgiPathDirective;
}
bool Location::hasClientMaxBodySizeDirective() const
{
	return _hasClientMaxBodySizeDirective;
}
bool Location::hasKeepAliveDirective() const
{
	return _hasKeepAliveDirective;
}
bool Location::hasRedirectDirective() const
{
	return _hasRedirectDirective;
}
bool Location::hasIndexDirective() const
{
	return _hasIndexDirective;
}
bool Location::hasStatusPagesDirective() const
{
	return _hasStatusPagesDirective;
}
bool Location::hasAllowedMethodsDirective() const
{
	return _hasAllowedMethodsDirective;
}

/*
** --------------------------------- DIRECTIVE INVESTIGATORS ---------------------------------
*/

bool Location::hasIndex(const std::string &index) const
{
	return _indexes.contains(index);
}
bool Location::hasStatusPage(const int &status) const
{
	return _statusPages.find(status) != _statusPages.end();
}
bool Location::hasAllowedMethod(const std::string &allowedMethod) const
{
	return std::find(_allowedMethods.begin(), _allowedMethods.end(), allowedMethod) != _allowedMethods.end();
}

/*
** --------------------------------- ACCESSORS ---------------------------------
*/

const std::string &Location::getRootPath() const
{
	return _rootPath;
}
const std::vector<std::string> &Location::getAllowedMethods() const
{
	return _allowedMethods;
}

const std::pair<int, std::string> &Location::getRedirect() const
{
	return _redirect;
}

const std::map<int, std::string> &Location::getStatusPaths() const
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

bool Location::getAutoIndexValue() const
{
	return _autoIndexValue;
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
	_hasRootDirective = true;
	_root = root;
}

void Location::insertAllowedMethod(const std::string &allowedMethod)
{
	_hasAllowedMethodsDirective = true;
	_allowedMethods.push_back(allowedMethod);
}

void Location::setAllowedMethods(const std::vector<std::string> &allowedMethods)
{
	_hasAllowedMethodsDirective = true;
	_allowedMethods = allowedMethods;
}

void Location::setStatusPages(const std::map<int, std::string> &statusPages)
{
	_statusPages = statusPages;
	_hasStatusPagesDirective = true;
}

void Location::insertStatusPage(const std::vector<int> &codes, const std::string &path)
{
	for (std::vector<int>::const_iterator it = codes.begin(); it != codes.end(); ++it)
	{
		_statusPages.insert(std::make_pair(*it, path));
	}
	_hasStatusPagesDirective = true;
}

void Location::setRedirect(const std::pair<int, std::string> &redirect)
{
	_redirect = redirect;
	_hasRedirectDirective = true;
}

void Location::setAutoIndex(const bool &autoIndex)
{
	_autoIndexValue = autoIndex;
	_hasAutoIndexDirective = true;
}

void Location::insertIndex(const std::string &index)
{
	_indexes.insert(index, index);
	_hasIndexesDirective = true;
}

void Location::setCgiPath(const std::string &cgiPath)
{
	_cgiPath = cgiPath;
	_hasCgiPathDirective = true;
}

void Location::setClientMaxBodySize(double size)
{
	_clientMaxBodySize = size;
	_hasClientMaxBodySizeDirective = true;
}

/*
** --------------------------------- UTILITY ---------------------------------
*/

bool Location::wasModified() const
{
	return _hasAllowedMethodsDirective || _hasStatusPagesDirective || _hasRedirectDirective || _hasIndexDirective ||
		   _hasCgiPathDirective || _hasClientMaxBodySizeDirective || _hasRootPathDirective || _hasAutoIndexDirective;
}

void Location::reset()
{
	// Directives
	_rootPath.clear();
	_allowedMethods.clear();
	_statusPages.clear();
	_redirect = std::pair<int, std::string>();
	_indexes.clear();
	_autoIndexValue = false;
	_cgiPath.clear();
	_clientMaxBodySize = -1.0;

	// Flags
	_hasRootPathDirective = false;
	_hasAutoIndexDirective = false;
	_hasCgiPathDirective = false;
	_hasClientMaxBodySizeDirective = false;
	_hasKeepAliveDirective = false;
	_hasRedirectDirective = false;
	_hasIndexDirective = false;
	_hasStatusPagesDirective = false;
	_hasAllowedMethodsDirective = false;
}

/* ************************************************************************** */
