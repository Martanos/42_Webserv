#include "../../includes/Config/Directives.hpp"
#include "../../includes/Http/HTTP.hpp"
#include <algorithm>

// Default constructor: initialize directive members and flags
Directives::Directives()
	: _rootPath(""), _autoIndexValue(false), _isCgiPathValue(false), _uploadPath(""),
	  _keepAliveValue(HTTP::DEFAULT_KEEP_ALIVE), _clientMaxBodySize(HTTP::DEFAULT_CLIENT_MAX_BODY_SIZE),
	  _redirect(std::pair<int, std::string>()), _hasRootPathDirective(false), _hasAutoIndexDirective(false),
	  _hasisCgiPathDirective(false), _hasUploadPathDirective(false), _hasKeepAliveDirective(false),
	  _hasClientMaxBodySizeDirective(false), _hasRedirectDirective(false), _hasIndexDirective(false),
	  _hasStatusPathDirective(false), _hasAllowedMethodsDirective(false)
{
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Directives::~Directives()
{
}

/*
** --------------------------------- DIRECTIVE FLAGS
* ---------------------------------
*/

bool Directives::hasRootPathDirective() const
{
	return (_hasRootPathDirective);
}
bool Directives::hasAutoIndexDirective() const
{
	return (_hasAutoIndexDirective);
}
bool Directives::hasisCgiPathDirective() const
{
	return (_hasisCgiPathDirective);
}
bool Directives::hasUploadPathDirective() const
{
	return (_hasUploadPathDirective);
}
bool Directives::hasKeepAliveDirective() const
{
	return (_hasKeepAliveDirective);
}
bool Directives::hasClientMaxBodySizeDirective() const
{
	return (_hasClientMaxBodySizeDirective);
}
bool Directives::hasRedirectDirective() const
{
	return (_hasRedirectDirective);
}
bool Directives::hasIndexDirective() const
{
	return (_hasIndexDirective);
}
bool Directives::hasStatusPathDirective() const
{
	return (_hasStatusPathDirective);
}
bool Directives::hasAllowedMethodsDirective() const
{
	return (_hasAllowedMethodsDirective);
}

/*
** --------------------------------- DIRECTIVE INVESTIGATORS
* ---------------------------------
*/

Directives::LocationType Directives::getLocationType() const
{
	return (_locationType);
}

bool Directives::hasIndex(const std::string &index) const
{
	return (std::find(_indexes.begin(), _indexes.end(), index) != _indexes.end());
}
bool Directives::hasStatusPath(int status) const
{
	return (_statusPaths.find(status) != _statusPaths.end());
}
bool Directives::hasAllowedMethod(const std::string &allowedMethod) const
{
	return (std::find(_allowedMethods.begin(), _allowedMethods.end(), allowedMethod) != _allowedMethods.end());
}

/*
** --------------------------------- ACCESSORS ---------------------------------
*/

const std::string *Directives::getRootPath() const
{
	return (&_rootPath);
}
bool Directives::getAutoIndexValue() const
{
	return (_autoIndexValue);
}
bool Directives::getisCgiPathValue() const
{
	return (_isCgiPathValue);
}
const std::string *Directives::getUploadPath() const
{
	return (&_uploadPath);
}
bool Directives::getKeepAliveValue() const
{
	return (_keepAliveValue);
}
double Directives::getClientMaxBodySize() const
{
	return (_clientMaxBodySize);
}
const std::pair<int, std::string> *Directives::getRedirect() const
{
	return (&_redirect);
}
const std::vector<std::string> *Directives::getIndexes() const
{
	return (&_indexes);
}
const std::string *Directives::getStatusPath(int status) const
{
	for (std::map<int, std::string>::const_iterator it = _statusPaths.begin(); it != _statusPaths.end(); ++it)
	{
		if (it->first == status)
			return (&(it->second));
	}
	return (NULL);
}
const std::map<int, std::string> *Directives::getStatusPaths() const
{
	return (&_statusPaths);
}
const std::vector<std::string> *Directives::getAllowedMethods() const
{
	return (&_allowedMethods);
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
void Directives::setIsCgiPath(bool isCgiPath)
{
	_locationType = isCgiPath ? CGI : STATIC;
	_isCgiPathValue = isCgiPath;
	_hasisCgiPathDirective = true;
}
void Directives::setUploadPath(const std::string &uploadPath)
{
	_locationType = UPLOAD;
	_uploadPath = uploadPath;
	_hasUploadPathDirective = true;
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
	_locationType = REDIRECT;
	_redirect = redirect;
	_hasRedirectDirective = true;
}
void Directives::insertIndex(const std::string &index)
{
	_indexes.push_back(index);
	_hasIndexDirective = true;
}
void Directives::setIndexes(const std::vector<std::string> &indexes)
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
