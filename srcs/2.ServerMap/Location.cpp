<<<<<<< HEAD
#include "../../includes/Location.hpp"
#include "../../includes/ConfigUtils.hpp"
#include "../../includes/StringUtils.hpp"
=======
#include "../../includes/Core/Location.hpp"
#include <algorithm>
#include <iostream>
>>>>>>> ConfigParserRefactor

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

<<<<<<< HEAD
Location::Location() : _autoIndex(false)
=======
Location::Location(const std::string &path)
>>>>>>> ConfigParserRefactor
{
	// Identifier members
	_path = path;
	_root = std::string();
	_allowedMethods = std::vector<std::string>();
<<<<<<< HEAD
	_redirect = "";
	_index = "";
	_cgiPath = "";
	_cgiParams = std::map<std::string, std::string>();
	_uploadPath = "";
=======
	_statusPages = std::map<int, std::string>();
	_redirect = std::pair<int, std::string>();
	_indexes = TrieTree<std::string>();
	_cgiPath = std::string();

	// Flags
	_modified = false;
>>>>>>> ConfigParserRefactor
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
<<<<<<< HEAD
	o << "Redirect: " << i.getRedirect() << std::endl;
	o << "AutoIndex: " << (i.isAutoIndexEnabled() ? "on" : "off") << std::endl;
	o << "Index: " << i.getIndex() << std::endl;
	o << "CgiPath: " << i.getCgiPath() << std::endl;
	o << "CgiParams: ";
	for (std::map<std::string, std::string>::const_iterator it = i.getCgiParams().begin(); it != i.getCgiParams().end();
=======
	o << "Redirect: " << i.getRedirect().first << " " << i.getRedirect().second << std::endl;
	o << "Status pages: ";
	for (std::map<int, std::string>::const_iterator it = i.getStatusPages().begin(); it != i.getStatusPages().end();
>>>>>>> ConfigParserRefactor
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

<<<<<<< HEAD
bool Location::isAutoIndexEnabled() const
=======
const std::map<int, std::string> &Location::getStatusPages() const
>>>>>>> ConfigParserRefactor
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
<<<<<<< HEAD
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

// void Location::addAllowedMethod(const std::string &allowedMethod)
// {
// 	if (std::find(_allowedMethods.begin(), _allowedMethods.end(), allowedMethod) == _allowedMethods.end())
// 		_allowedMethods.push_back(allowedMethod);
// 	else
// 		Logger::log(Logger::WARNING,
// 					"Allowed method " + allowedMethod + " already exists in location " + _path + " ignoring duplicate");
// }

// void Location::addCgiParam(const std::string &cgiParam, const std::string &value)
// {
// 	if (_cgiParams.find(cgiParam) == _cgiParams.end())
// 		_cgiParams[cgiParam] = value;
// 	else
// 		Logger::log(Logger::WARNING,
// 					"Cgi param " + cgiParam + " already exists in location " + _path + " ignoring duplicate");
// }

// Operators
bool Location::operator==(const Location &other) const
{
	return _path == other._path && _root == other._root && _allowedMethods == other._allowedMethods &&
		   _redirect == other._redirect && _autoIndex == other._autoIndex && _index == other._index &&
		   _cgiPath == other._cgiPath && _cgiParams == other._cgiParams &&
		   _uploadPath == other._uploadPath;
}


// Parsing methods
void Location::addPath(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream pathStream(line);
	ConfigUtils::validateDirective(pathStream, "location", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(pathStream >> token) || token.empty())
		ConfigUtils::throwConfigError("Empty location path at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);

	_path = token;
}

void Location::addRoot(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream rootStream(line);
	ConfigUtils::validateDirective(rootStream, "root", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(rootStream >> token) || token.empty())
		ConfigUtils::throwConfigError("Empty root path at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);

	_root = token;
}

void Location::addAllowedMethods(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream methodsStream(line);
	ConfigUtils::validateDirective(methodsStream, "allowed_methods", lineNumber, __FILE__, __LINE__);

	// Clear existing methods and read new ones
	_allowedMethods.clear();
	std::string token;
	while (methodsStream >> token)
	{
		if (token == "GET" || token == "POST" || token == "DELETE")
		{
			if (std::find(_allowedMethods.begin(), _allowedMethods.end(), token) == _allowedMethods.end())
			{
				_allowedMethods.push_back(token);
			}
			else
			{
				ConfigUtils::throwConfigError("Duplicate method '" + token + "' at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
			}
		}
		else
		{
			ConfigUtils::throwConfigError("Invalid HTTP method '" + token + "' at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
		}
	}

	if (_allowedMethods.empty())
	{
		ConfigUtils::throwConfigError("No methods specified in allow_methods directive at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
	}
}

void Location::addRedirect(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream redirectStream(line);
	ConfigUtils::validateDirective(redirectStream, "return", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(redirectStream >> token) || token.empty())
		ConfigUtils::throwConfigError("Empty redirect URL at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);

	_redirect = token;
}

void Location::addAutoIndex(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream autoIndexStream(line);
	ConfigUtils::validateDirective(autoIndexStream, "autoindex", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(autoIndexStream >> token) || (token != "on" && token != "off"))
		ConfigUtils::throwConfigError("Invalid autoindex value at line " + StringUtils::toString(lineNumber) + ": " + token, __FILE__, __LINE__);

	_autoIndex = (token == "on");
}

void Location::addIndex(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream indexStream(line);
	ConfigUtils::validateDirective(indexStream, "index", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(indexStream >> token) || token.empty())
		ConfigUtils::throwConfigError("Empty index file at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);

	_index = token;
}

void Location::addCgiPath(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream cgiPathStream(line);
	ConfigUtils::validateDirective(cgiPathStream, "cgi_path", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(cgiPathStream >> token) || token.empty())
		ConfigUtils::throwConfigError("Empty CGI path at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);

	_cgiPath = token;
}

// void Location::addCgiIndex(std::string line, double lineNumber)
// {
// 	lineValidation(line, lineNumber);

// 	std::stringstream cgiIndexStream(line);
// 	validateDirective(cgiIndexStream, "cgi_index", lineNumber, __FILE__, __LINE__);

// 	std::string token;
// 	if (!(cgiIndexStream >> token) || token.empty())
// 		throwConfigError("Empty CGI index at line " + Location::numberToString(lineNumber), __FILE__, __LINE__);

// 	_cgiIndex = token;
// }

void Location::addCgiParam(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream cgiParamStream(line);
	ConfigUtils::validateDirective(cgiParamStream, "cgi_param", lineNumber, __FILE__, __LINE__);

	std::string param, value;
	if (!(cgiParamStream >> param) || param.empty())
		ConfigUtils::throwConfigError("Empty CGI parameter name at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);

	if (!(cgiParamStream >> value) || value.empty())
		ConfigUtils::throwConfigError("Empty CGI parameter value at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);

	_cgiParams[param] = value;
}

void Location::addUploadPath(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream uploadPathStream(line);
	ConfigUtils::validateDirective(uploadPathStream, "upload_path", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(uploadPathStream >> token) || token.empty())
		ConfigUtils::throwConfigError("Empty upload path at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);

	_uploadPath = token;
}

// Debugging methods
void Location::printConfig() const
{
	std::cout << "    Location Configuration:" << std::endl;
	std::cout << "      Path: " << _path << std::endl;
	std::cout << "      Root: " << _root << std::endl;

	std::cout << "      Allowed Methods: ";
	if (!_allowedMethods.empty())
	{
		for (size_t i = 0; i < _allowedMethods.size(); ++i)
		{
			if (i > 0)
				std::cout << ", ";
			std::cout << _allowedMethods[i];
		}
	}
	else
	{
		std::cout << "(none)";
	}
	std::cout << std::endl;

	std::cout << "      Redirect: " << _redirect << std::endl;
	std::cout << "      Auto Index: " << (_autoIndex ? "on" : "off") << std::endl;
	std::cout << "      Index: " << _index << std::endl;
	std::cout << "      CGI Path: " << _cgiPath << std::endl;
	// std::cout << "      CGI Index: " << _cgiIndex << std::endl;

	std::cout << "      CGI Parameters: ";
	if (!_cgiParams.empty())
	{
		std::cout << std::endl;
		for (std::map<std::string, std::string>::const_iterator it = _cgiParams.begin(); it != _cgiParams.end(); ++it)
		{
			std::cout << "        " << it->first << " = " << it->second << std::endl;
		}
	}
	else
	{
		std::cout << "(none)" << std::endl;
	}

	std::cout << "      Upload Path: " << _uploadPath << std::endl;
=======
	_modified = true;
>>>>>>> ConfigParserRefactor
}

/* ************************************************************************** */
