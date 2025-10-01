#include "../../includes/LocationConfig.hpp"
#include "../../includes/ConfigParser.hpp"

// Private static utilities
void LocationConfig::throwConfigError(const std::string& msg, const char* file, int line)
{
	Logger::log(Logger::ERROR, msg, file, line);
	throw std::runtime_error(msg);
}

bool LocationConfig::validateDirective(std::stringstream& stream, const std::string& expectedDirective, double lineNumber, const char* file, int line)
{
	std::string token;
	if (!(stream >> token) || token != expectedDirective)
	{
		std::stringstream ss;
		ss << "Invalid " << expectedDirective << " directive at line " << lineNumber;
		throwConfigError(ss.str(), file, line);
	}
	return true;
}

void LocationConfig::lineValidation(std::string &line, int lineNumber)
{
	if (line.empty())
	{
	throwConfigError("Empty directive at line " + ConfigParser::numberToString(lineNumber), __FILE__, __LINE__);
	}
	else if (line.find_last_of(';') != line.length() - 1)
	{
		throwConfigError("Expected semicolon at the end of line " + ConfigParser::numberToString(lineNumber) + " : " + line, __FILE__, __LINE__);
	}
	line.erase(line.find_last_of(';'));
}

// Constructors
LocationConfig::LocationConfig() : _autoIndex(false)
{
}

// Destructor
LocationConfig::~LocationConfig()
{
}

// Copy constructor
LocationConfig::LocationConfig(const LocationConfig &other)
{
	_path = other._path;
	_root = other._root;
	_allowedMethods = other._allowedMethods;
	_redirect = other._redirect;
	_autoIndex = other._autoIndex;
	_index = other._index;
	_cgiPath = other._cgiPath;
	_cgiIndex = other._cgiIndex;
	_cgiParams = other._cgiParams;
	_uploadPath = other._uploadPath;
}

// Assignment operator
LocationConfig &LocationConfig::operator=(const LocationConfig &other)
{
	if (this != &other)
	{
		_path = other._path;
		_root = other._root;
		_allowedMethods = other._allowedMethods;
		_redirect = other._redirect;
		_autoIndex = other._autoIndex;
		_index = other._index;
		_cgiPath = other._cgiPath;
		_cgiIndex = other._cgiIndex;
		_cgiParams = other._cgiParams;
		_uploadPath = other._uploadPath;
	}
	return *this;
}

// Equality operator
bool LocationConfig::operator==(const LocationConfig &other) const
{
	return _path == other._path && _root == other._root && _allowedMethods == other._allowedMethods &&
		   _redirect == other._redirect && _autoIndex == other._autoIndex && _index == other._index &&
		   _cgiPath == other._cgiPath && _cgiIndex == other._cgiIndex && _cgiParams == other._cgiParams &&
		   _uploadPath == other._uploadPath;
}

// Getters
const std::string &LocationConfig::getPath() const
{
	return _path;
}
const std::string &LocationConfig::getRoot() const
{
	return _root;
}
const std::vector<std::string> &LocationConfig::getAllowedMethods() const
{
	return _allowedMethods;
}
const std::string &LocationConfig::getRedirect() const
{
	return _redirect;
}
bool LocationConfig::isAutoIndexEnabled() const
{
	return _autoIndex;
}
const std::string &LocationConfig::getIndex() const
{
	return _index;
}
const std::string &LocationConfig::getCgiPath() const
{
	return _cgiPath;
}
const std::string &LocationConfig::getCgiIndex() const
{
	return _cgiIndex;
}
const std::map<std::string, std::string> &LocationConfig::getCgiParams() const
{
	return _cgiParams;
}
const std::string &LocationConfig::getUploadPath() const
{
	return _uploadPath;
}
// Parsing methods
void LocationConfig::addPath(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream pathStream(line);
	validateDirective(pathStream, "location", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(pathStream >> token) || token.empty())
	{
		throwConfigError("Empty location path at line " + ConfigParser::numberToString(lineNumber), __FILE__, __LINE__);
	}

	_path = token;
}

void LocationConfig::addRoot(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream rootStream(line);
	validateDirective(rootStream, "root", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(rootStream >> token) || token.empty())
	{
		throwConfigError("Empty root path at line " + ConfigParser::numberToString(lineNumber), __FILE__, __LINE__);
	}

	_root = token;
}

void LocationConfig::addAllowedMethods(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream methodsStream(line);
	validateDirective(methodsStream, "allowed_methods", lineNumber, __FILE__, __LINE__);

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
				throwConfigError("Duplicate method '" + token + "' at line " + ConfigParser::numberToString(lineNumber), __FILE__, __LINE__);
			}
		}
		else
		{
			throwConfigError("Invalid HTTP method '" + token + "' at line " + ConfigParser::numberToString(lineNumber), __FILE__, __LINE__);
		}
	}

	if (_allowedMethods.empty())
	{
		throwConfigError("No methods specified in allow_methods directive at line " + ConfigParser::numberToString(lineNumber), __FILE__, __LINE__);
	}
}

void LocationConfig::addRedirect(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream redirectStream(line);
	validateDirective(redirectStream, "return", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(redirectStream >> token) || token.empty())
	{
		throwConfigError("Empty redirect URL at line " + ConfigParser::numberToString(lineNumber), __FILE__, __LINE__);
	}

	_redirect = token;
}

void LocationConfig::addAutoIndex(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream autoIndexStream(line);
	validateDirective(autoIndexStream, "autoindex", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(autoIndexStream >> token) || (token != "on" && token != "off"))
	{
		throwConfigError("Invalid autoindex value at line " + ConfigParser::numberToString(lineNumber) + ": " + token, __FILE__, __LINE__);
	}

	_autoIndex = (token == "on");
}

void LocationConfig::addIndex(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream indexStream(line);
	validateDirective(indexStream, "index", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(indexStream >> token) || token.empty())
	{
		throwConfigError("Empty index file at line " + ConfigParser::numberToString(lineNumber), __FILE__, __LINE__);
	}

	_index = token;
}

void LocationConfig::addCgiPath(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream cgiPathStream(line);
	validateDirective(cgiPathStream, "cgi_path", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(cgiPathStream >> token) || token.empty())
	{
		throwConfigError("Empty CGI path at line " + ConfigParser::numberToString(lineNumber), __FILE__, __LINE__);
	}

	_cgiPath = token;
}

void LocationConfig::addCgiIndex(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream cgiIndexStream(line);
	validateDirective(cgiIndexStream, "cgi_index", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(cgiIndexStream >> token) || token.empty())
	{
		throwConfigError("Empty CGI index at line " + ConfigParser::numberToString(lineNumber), __FILE__, __LINE__);
	}

	_cgiIndex = token;
}

void LocationConfig::addCgiParam(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream cgiParamStream(line);
	validateDirective(cgiParamStream, "cgi_param", lineNumber, __FILE__, __LINE__);

	std::string param, value;
	if (!(cgiParamStream >> param) || param.empty())
	{
		throwConfigError("Empty CGI parameter name at line " + ConfigParser::numberToString(lineNumber), __FILE__, __LINE__);
	}

	if (!(cgiParamStream >> value) || value.empty())
	{
		throwConfigError("Empty CGI parameter value at line " + ConfigParser::numberToString(lineNumber), __FILE__, __LINE__);
	}

	_cgiParams[param] = value;
}

void LocationConfig::addUploadPath(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream uploadPathStream(line);
	validateDirective(uploadPathStream, "upload_path", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(uploadPathStream >> token) || token.empty())
	{
		throwConfigError("Empty upload path at line " + ConfigParser::numberToString(lineNumber), __FILE__, __LINE__);
	}

	_uploadPath = token;
}

// Debugging methods
void LocationConfig::printConfig() const
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
	std::cout << "      CGI Index: " << _cgiIndex << std::endl;

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
}
