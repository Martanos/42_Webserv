#include "../../includes/ConfObjs/Location.hpp"
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
		_statusPaths = rhs._statusPaths;
		_allowedMethods = rhs._allowedMethods;

		// Flags
		_hasRootPathDirective = rhs._hasRootPathDirective;
		_hasAutoIndexDirective = rhs._hasAutoIndexDirective;
		_hasCgiPathDirective = rhs._hasCgiPathDirective;
		_hasClientMaxBodySizeDirective = rhs._hasClientMaxBodySizeDirective;
		_hasKeepAliveDirective = rhs._hasKeepAliveDirective;
		_hasRedirectDirective = rhs._hasRedirectDirective;
		_hasIndexDirective = rhs._hasIndexDirective;
		_hasStatusPathDirective = rhs._hasStatusPathDirective;
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
	else
		o << "Root: (not set)" << std::endl;
	if (i.hasAutoIndexDirective())
		o << "AutoIndex: " << (i.getAutoIndexValue() ? "on" : "off") << std::endl;
	else
		o << "AutoIndex: (not set)" << std::endl;
	if (i.hasCgiPathDirective())
		o << "CgiPath: " << i.getCgiPath() << std::endl;
	else
		o << "CgiPath: (not set)" << std::endl;
	if (i.hasClientMaxBodySizeDirective())
		o << "Client Max Body Size: " << i.getClientMaxBodySize() << std::endl;
	else
		o << "Client Max Body Size: (not set)" << std::endl;
	if (i.hasKeepAliveDirective())
		o << "Keep Alive: " << (i.getKeepAliveValue() ? "on" : "off") << std::endl;
	else
		o << "Keep Alive: (not set)" << std::endl;
	if (i.hasRedirectDirective())
	{
		const std::pair<int, std::string> &redirect = i.getRedirect();
		o << "Redirect: " << redirect.first << " -> " << redirect.second << std::endl;
	}
	else
		o << "Redirect: (not set)" << std::endl;
	if (i.hasIndexDirective())
	{
		o << "Indexes: " << std::endl;
		const std::vector<std::string> &indexes = i.getIndexes().getAllKeys();
		for (size_t idx = 0; idx < indexes.size(); ++idx)
		{
			o << "  " << indexes[idx] << std::endl;
		}
	}
	else
	{
		o << "Indexes: (not set)" << std::endl;
	}
	if (i.hasStatusPathDirective())
	{
		o << "Status Pages: " << std::endl;
		const std::map<int, std::string> &statusPages = i.getStatusPaths();
		for (std::map<int, std::string>::const_iterator it = statusPages.begin(); it != statusPages.end(); ++it)
		{
			o << "  " << it->first << " -> " << it->second << std::endl;
		}
	}
	else
	{
		o << "Status Pages: (not set)" << std::endl;
	}
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
	else
	{
		o << "Allowed Methods: (not set)" << std::endl;
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
** --------------------------------- UTILITY ---------------------------------
*/

bool Location::wasModified() const
{
	return _hasAllowedMethodsDirective || _hasStatusPathDirective || _hasRedirectDirective || _hasIndexDirective ||
		   _hasCgiPathDirective || _hasClientMaxBodySizeDirective || _hasRootPathDirective || _hasAutoIndexDirective;
}

void Location::reset()
{
	// Directives
	_rootPath.clear();
	_allowedMethods.clear();
	_statusPaths.clear();
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
	_hasStatusPathDirective = false;
	_hasAllowedMethodsDirective = false;
}

/* ************************************************************************** */
