#include "Location.hpp"

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
	_index = "";
	_cgiPath = "";
	_cgiParams = std::map<std::string, std::string>();
	_uploadPath = "";
}

Location::Location(const Location &src)
{
	if (this != &src)
	{
		_path = src._path;
		_root = src._root;
		_allowedMethods = src._allowedMethods;
		_redirect = src._redirect;
		_autoIndex = src._autoIndex;
		_index = src._index;
		_cgiPath = src._cgiPath;
		_cgiParams = src._cgiParams;
		_uploadPath = src._uploadPath;
	}
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
		_index = rhs._index;
		_cgiPath = rhs._cgiPath;
		_cgiParams = rhs._cgiParams;
		_uploadPath = rhs._uploadPath;
	}
	return *this;
}

std::ostream &operator<<(std::ostream &o, Location const &i)
{
	o << "--------------------------------" << std::endl;
	o << "Path: " << i.getPath() << std::endl;
	o << "Root: " << i.getRoot() << std::endl;
	o << "Allowed Methods: ";
	for (std::vector<std::string>::const_iterator it = i.getAllowedMethods().begin(); it != i.getAllowedMethods().end(); ++it)
		o << *it << " ";
	o << std::endl;
	o << "Redirect: " << i.getRedirect() << std::endl;
	o << "AutoIndex: " << i.getAutoIndex() << std::endl;
	o << "Index: " << i.getIndex() << std::endl;
	o << "CgiPath: " << i.getCgiPath() << std::endl;
	o << "CgiParams: ";
	for (std::map<std::string, std::string>::const_iterator it = i.getCgiParams().begin(); it != i.getCgiParams().end(); ++it)
		o << it->first << "=" << it->second << " ";
	o << std::endl;
	o << "UploadPath: " << i.getUploadPath() << std::endl;
	o << "--------------------------------" << std::endl;
	return o;
}

/*
** --------------------------------- GETTERS ---------------------------------
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

const std::string &Location::getRedirect() const
{
	return _redirect;
}

const bool &Location::getAutoIndex() const
{
	return _autoIndex;
}

const std::string &Location::getIndex() const
{
	return _index;
}

const std::string &Location::getCgiPath() const
{
	return _cgiPath;
}

const std::map<std::string, std::string> &Location::getCgiParams() const
{
	return _cgiParams;
}

const std::string &Location::getUploadPath() const
{
	return _uploadPath;
}

/*
** --------------------------------- SETTERS ---------------------------------
*/

void Location::setPath(const std::string &path)
{
	_path = path;
}

void Location::setRoot(const std::string &root)
{
	_root = root;
}

void Location::setAllowedMethods(const std::vector<std::string> &allowedMethods)
{
	_allowedMethods = allowedMethods;
}

void Location::setRedirect(const std::string &redirect)
{
	_redirect = redirect;
}

void Location::setAutoIndex(const bool &autoIndex)
{
	_autoIndex = autoIndex;
}

void Location::setIndex(const std::string &index)
{
	_index = index;
}

void Location::setCgiPath(const std::string &cgiPath)
{
	_cgiPath = cgiPath;
}

void Location::setCgiParams(const std::map<std::string, std::string> &cgiParams)
{
	_cgiParams = cgiParams;
}

void Location::setUploadPath(const std::string &uploadPath)
{
	_uploadPath = uploadPath;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void Location::addAllowedMethod(const std::string &allowedMethod)
{
	if (std::find(_allowedMethods.begin(), _allowedMethods.end(), allowedMethod) == _allowedMethods.end())
		_allowedMethods.push_back(allowedMethod);
	else
		Logger::log(Logger::WARNING, "Allowed method " + allowedMethod + " already exists in location " + _path + " ignoring duplicate");
}

void Location::addCgiParam(const std::string &cgiParam, const std::string &value)
{
	if (_cgiParams.find(cgiParam) == _cgiParams.end())
		_cgiParams[cgiParam] = value;
	else
		Logger::log(Logger::WARNING, "Cgi param " + cgiParam + " already exists in location " + _path + " ignoring duplicate");
}

/* ************************************************************************** */
