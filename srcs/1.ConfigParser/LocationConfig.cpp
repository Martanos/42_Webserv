#include "../../includes/LocationConfig.hpp"

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
	std::string token;
	std::stringstream errorMessage;

	if (!(pathStream >> token) || token != "location")
	{
		errorMessage << "Invalid location directive at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	if (!(pathStream >> token) || token.empty())
	{
		errorMessage << "Empty location path at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	_path = token;
}

void LocationConfig::addRoot(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream rootStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(rootStream >> token) || token != "root")
	{
		errorMessage << "Invalid root directive at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	if (!(rootStream >> token) || token.empty())
	{
		errorMessage << "Empty root path at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	_root = token;
}

void LocationConfig::addAllowedMethods(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream methodsStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(methodsStream >> token) || token != "allowed_methods")
	{
		errorMessage << "Invalid allow_methods directive at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	// Clear existing methods and read new ones
	_allowedMethods.clear();
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
				errorMessage << "Duplicate method '" << token << "' at line " << lineNumber;
				Logger::log(Logger::ERROR, errorMessage.str());
				throw std::runtime_error(errorMessage.str());
			}
		}
		else
		{
			errorMessage << "Invalid HTTP method '" << token << "' at line " << lineNumber;
			Logger::log(Logger::ERROR, errorMessage.str());
			throw std::runtime_error(errorMessage.str());
		}
	}

	if (_allowedMethods.empty())
	{
		errorMessage << "No methods specified in allow_methods directive at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}
}

void LocationConfig::addRedirect(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream redirectStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(redirectStream >> token) || token != "return")
	{
		errorMessage << "Invalid return directive at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	if (!(redirectStream >> token) || token.empty())
	{
		errorMessage << "Empty redirect URL at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	_redirect = token;
}

void LocationConfig::addAutoIndex(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream autoIndexStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(autoIndexStream >> token) || token != "autoindex")
	{
		errorMessage << "Invalid autoindex directive at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	if (!(autoIndexStream >> token) || (token != "on" && token != "off"))
	{
		errorMessage << "Invalid autoindex value at line " << lineNumber << ": " << token;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	_autoIndex = (token == "on");
}

void LocationConfig::addIndex(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream indexStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(indexStream >> token) || token != "index")
	{
		errorMessage << "Invalid index directive at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	if (!(indexStream >> token) || token.empty())
	{
		errorMessage << "Empty index file at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	_index = token;
}

void LocationConfig::addCgiPath(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream cgiPathStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(cgiPathStream >> token) || token != "cgi_path")
	{
		errorMessage << "Invalid cgi_path directive at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	if (!(cgiPathStream >> token) || token.empty())
	{
		errorMessage << "Empty CGI path at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	_cgiPath = token;
}

void LocationConfig::addCgiIndex(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream cgiIndexStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(cgiIndexStream >> token) || token != "cgi_index")
	{
		errorMessage << "Invalid cgi_index directive at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	if (!(cgiIndexStream >> token) || token.empty())
	{
		errorMessage << "Empty CGI index at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	_cgiIndex = token;
}

void LocationConfig::addCgiParam(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream cgiParamStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(cgiParamStream >> token) || token != "cgi_param")
	{
		errorMessage << "Invalid cgi_param directive at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	std::string param, value;
	if (!(cgiParamStream >> param) || param.empty())
	{
		errorMessage << "Empty CGI parameter name at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	if (!(cgiParamStream >> value) || value.empty())
	{
		errorMessage << "Empty CGI parameter value at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	_cgiParams[param] = value;
}

void LocationConfig::addUploadPath(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream uploadPathStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(uploadPathStream >> token) || token != "upload_path")
	{
		errorMessage << "Invalid upload_path directive at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}

	if (!(uploadPathStream >> token) || token.empty())
	{
		errorMessage << "Empty upload path at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
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

// Utils
void LocationConfig::lineValidation(std::string &line, int lineNumber)
{
	std::stringstream errorMessage;
	if (line.empty())
	{
		errorMessage << "Empty directive at line " << lineNumber;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}
	else if (line.find_last_of(';') != line.length() - 1)
	{
		errorMessage << "Expected semicolon at the end of line " << lineNumber << " : " << line;
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}
	line.erase(line.find_last_of(';'));
}
