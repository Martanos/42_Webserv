#include "../../includes/Config/Directives.hpp"

// Default constructor: initialize directive members and flags
Directives::Directives()
	: _rootPath(), _autoIndexValue(false), _cgiPath(), _clientMaxBodySize(-1.0), _keepAliveValue(false), _redirect(),
	  _indexes(), _statusPaths(), _allowedMethods(), _hasRootPathDirective(false), _hasAutoIndexDirective(false),
	  _hasCgiPathDirective(false), _hasClientMaxBodySizeDirective(false), _hasKeepAliveDirective(false),
	  _hasRedirectDirective(false), _hasIndexDirective(false), _hasStatusPathDirective(false),
	  _hasAllowedMethodsDirective(false)
{
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Directives::~Directives()
{
}

/*
** --------------------------------- DIRECTIVE FLAGS ---------------------------------
*/

bool Directives::hasRootPathDirective() const
{
	return _hasRootPathDirective;
}
bool Directives::hasAutoIndexDirective() const
{
	return _hasAutoIndexDirective;
}
bool Directives::hasCgiPathDirective() const
{
	return _hasCgiPathDirective;
}
bool Directives::hasClientMaxBodySizeDirective() const
{
	return _hasClientMaxBodySizeDirective;
}
bool Directives::hasKeepAliveDirective() const
{
	return _hasKeepAliveDirective;
}
bool Directives::hasRedirectDirective() const
{
	return _hasRedirectDirective;
}
bool Directives::hasIndexDirective() const
{
	return _hasIndexDirective;
}
bool Directives::hasStatusPathDirective() const
{
	return _hasStatusPathDirective;
}
bool Directives::hasAllowedMethodsDirective() const
{
	return _hasAllowedMethodsDirective;
}

/*
** --------------------------------- DIRECTIVE INVESTIGATORS ---------------------------------
*/

bool Directives::hasIndex(const std::string &index) const
{
	return _indexes.contains(index);
}
bool Directives::hasStatusPath(int status) const
{
	return _statusPaths.find(status) != _statusPaths.end();
}
bool Directives::hasAllowedMethod(const std::string &allowedMethod) const
{
	return std::find(_allowedMethods.begin(), _allowedMethods.end(), allowedMethod) != _allowedMethods.end();
}

/*
** --------------------------------- ACCESSORS ---------------------------------
*/

const std::string &Directives::getRootPath() const
{
	return _rootPath;
}
bool Directives::getAutoIndexValue() const
{
	return _autoIndexValue;
}
const std::string &Directives::getCgiPath() const
{
	return _cgiPath;
}
double Directives::getClientMaxBodySize() const
{
	return _clientMaxBodySize;
}
bool Directives::getKeepAliveValue() const
{
	return _keepAliveValue;
}
const std::pair<int, std::string> &Directives::getRedirect() const
{
	return _redirect;
}
const TrieTree<std::string> &Directives::getIndexes() const
{
	return _indexes;
}
const std::string &Directives::getStatusPath(int status) const
{
	std::map<int, std::string>::const_iterator it = _statusPaths.find(status);
	if (it != _statusPaths.end())
	{
		return it->second;
	}
	static const std::string emptyString;
	return emptyString;
}
const std::map<int, std::string> &Directives::getStatusPaths() const
{
	return _statusPaths;
}
const std::vector<std::string> &Directives::getAllowedMethods() const
{
	return _allowedMethods;
}

/*
** --------------------------------- Mutators ---------------------------------
*/
void Directives::setRootPath(const std::string &root)
{
	_hasRootPathDirective = true;
	_rootPath = root;
}
void Directives::setAutoIndex(bool autoIndex)
{
	_autoIndexValue = autoIndex;
	_hasAutoIndexDirective = true;
}
void Directives::setCgiPath(const std::string &cgiPath)
{
	_cgiPath = cgiPath;
	_hasCgiPathDirective = true;
}
void Directives::setClientMaxBodySize(double size)
{
	_clientMaxBodySize = size;
	_hasClientMaxBodySizeDirective = true;
}
void Directives::setKeepAlive(bool keepAlive)
{
	_keepAliveValue = keepAlive;
	_hasKeepAliveDirective = true;
}
void Directives::setRedirect(const std::pair<int, std::string> &redirect)
{
	_redirect = redirect;
	_hasRedirectDirective = true;
}
void Directives::insertIndex(const std::string &index)
{
	_indexes.insert(index, index);
	_hasIndexDirective = true;
}
void Directives::setIndexes(const TrieTree<std::string> &indexes)
{
	_indexes = indexes;
	_hasIndexDirective = true;
}
void Directives::insertStatusPath(const std::vector<int> &codes, const std::string &path)
{
	for (std::vector<int>::const_iterator it = codes.begin(); it != codes.end(); ++it)
	{
		_statusPaths.insert(std::make_pair(*it, path));
	}
	_hasStatusPathDirective = true;
}
void Directives::setStatusPaths(const std::map<int, std::string> &statusPaths)
{
	_statusPaths = statusPaths;
	_hasStatusPathDirective = true;
}
void Directives::insertAllowedMethod(const std::string &allowedMethod)
{
	_hasAllowedMethodsDirective = true;
	_allowedMethods.push_back(allowedMethod);
}
void Directives::setAllowedMethods(const std::vector<std::string> &allowedMethods)
{
	_hasAllowedMethodsDirective = true;
	_allowedMethods = allowedMethods;
}
