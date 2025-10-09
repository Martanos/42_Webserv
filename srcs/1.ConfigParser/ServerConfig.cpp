#include "../../includes/ServerConfig.hpp"
#include "../../includes/ConfigParser.hpp"
#include "../../includes/Constants.hpp"
#include "../../includes/ConfigUtils.hpp"
#include "../../includes/StringUtils.hpp"



// Constructors
ServerConfig::ServerConfig()
{
	_maxUriSize = HTTP::MAX_URI_SIZE;
	_maxHeaderSize = HTTP::MAX_HEADER_SIZE;
	_clientMaxBodySize = SERVER::DEFAULT_CLIENT_MAX_BODY_SIZE;
	_autoindex = SERVER::DEFAULT_AUTOINDEX;
	_keepAlive = SERVER::DEFAULT_KEEP_ALIVE;
}

// Destructor
ServerConfig::~ServerConfig()
{
}

// Copy constructor
ServerConfig::ServerConfig(const ServerConfig &other)
{
	*this = other;
}

// Assignment operator
ServerConfig &ServerConfig::operator=(const ServerConfig &other)
{
	if (this != &other)
	{
		_serverNames = other._serverNames;
		_hosts_ports = other._hosts_ports;
		_root = other._root;
		_indexes = other._indexes;
		_autoindex = other._autoindex;
		_maxUriSize = other._maxUriSize;
		_maxHeaderSize = other._maxHeaderSize;
		_clientMaxBodySize = other._clientMaxBodySize;
		_statusPages = other._statusPages;
		_locations = other._locations;
		_accessLog = other._accessLog;
		_errorLog = other._errorLog;
		_keepAlive = other._keepAlive;
	}
	return *this;
}

// Getters
const std::vector<std::string> &ServerConfig::getServerNames() const
{
	return _serverNames;
}
const std::vector<std::pair<std::string, unsigned short> > &ServerConfig::getHosts_ports() const
{
	return _hosts_ports;
}
const std::string &ServerConfig::getRoot() const
{
	return _root;
}
const std::vector<std::string> &ServerConfig::getIndexes() const
{
	return _indexes;
}
bool ServerConfig::getAutoindex() const
{
	return _autoindex;
}
double ServerConfig::getMaxUriSize() const
{
	return _maxUriSize;
}
double ServerConfig::getMaxHeaderSize() const
{
	return _maxHeaderSize;
}
double ServerConfig::getClientMaxBodySize() const
{
	return _clientMaxBodySize;
}
const std::map<int, std::string> &ServerConfig::getStatusPages() const
{
	return _statusPages;
}
const std::vector<Location> &ServerConfig::getLocations() const
{
	return _locations;
}
const std::string &ServerConfig::getAccessLog() const
{
	return _accessLog;
}
const std::string &ServerConfig::getErrorLog() const
{
	return _errorLog;
}
bool ServerConfig::getKeepAlive() const
{
	return _keepAlive;
}



// Local utility functions removed - now use ConfigUtils
// Parsing methods
void ServerConfig::addServerName(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream serverNameStream(line);
	ConfigUtils::validateDirective(serverNameStream, "server_name", lineNumber, __FILE__, __LINE__);

	std::string token;
	// Server name validation and vector population
	while (serverNameStream >> token)
	{
		// Special cases
		if (token == "_") // catch all
		{
			if (std::find(this->_serverNames.begin(), this->_serverNames.end(), token) != this->_serverNames.end())
			{
				ConfigUtils::throwConfigError("Duplicate catch-all server_name detected: " + token + " at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
			}
			else
				_serverNames.push_back(token);
			continue;
		}
		// Simple verification if hostname is valid (no special characters)
		if (token.length() > 255)
		{
			ConfigUtils::throwConfigError("Invalid server_name at line " + StringUtils::toString(lineNumber) + ": " + token + " (Hostname exceeds 255 characters)", __FILE__, __LINE__);
		}
		else if (token.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJK"
										 "LMNOPQRSTUVWXYZ0123456789.-_") != std::string::npos)
		{
			ConfigUtils::throwConfigError("Invalid server_name at line " + StringUtils::toString(lineNumber) + ": " + token + " (Contains invalid characters)", __FILE__, __LINE__);
		}
		else if (!isalpha(token[0]) || !isalnum(token[token.length() - 1]))
		{
			ConfigUtils::throwConfigError("Invalid server_name at line " + StringUtils::toString(lineNumber) + ": " + token + " (Hostname must begin and end with alphanumeric character)", __FILE__, __LINE__);
		}
		else if (std::find(this->_serverNames.begin(), this->_serverNames.end(), token) != this->_serverNames.end())
		{
			ConfigUtils::throwConfigError("Duplicate server_name detected: " + token + " at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
		}
		else
			_serverNames.push_back(token);

		if (_serverNames.size() >= 255)
		{
			ConfigUtils::throwConfigError("Too many server_names at line " + StringUtils::toString(lineNumber) + " (Maximum of 255 server_names allowed)", __FILE__, __LINE__);
		}
	}
}

void ServerConfig::addHosts_ports(std::string line, double lineNumber)
{
	Logger::debug("ServerConfig: Processing listen directive: " + line);
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream hosts_portsStream(line);
	ConfigUtils::validateDirective(hosts_portsStream, "listen", lineNumber, __FILE__, __LINE__);

	std::string token;

	while (hosts_portsStream >> token)
	{
		std::pair<std::string, unsigned short> host_port;
		bool tokenProcessed = false;
		// First, check if token contains a colon (indicating host:port format)
		if (token.find(':') != std::string::npos)
		{
			host_port = split_host_port(token);
			if (host_port.first.empty() && host_port.second == 0)
			{
				ConfigUtils::throwConfigError("Invalid host:port format: " + token + " at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
			}
			tokenProcessed = true;
		}
		// If no colon, try to validate as port only
		else if (try_validate_port_only(token))
		{
			// std::cout << "Valid port only: " << token << std::endl;
			host_port = std::make_pair("0.0.0.0", std::strtol(token.c_str(), NULL, 10));
			tokenProcessed = true;
		}
		// If none of the above worked, it's an invalid token
		if (!tokenProcessed)
		{
			ConfigUtils::throwConfigError("Invalid listen directive: " + token + " at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
		}

		// Check for duplicates and add to the list
		if (std::find(this->_hosts_ports.begin(), this->_hosts_ports.end(), host_port) != this->_hosts_ports.end())
		{
			ConfigUtils::throwConfigError("Duplicate listen directive detected: " + token + " at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
		}
		else
		{
			_hosts_ports.push_back(host_port);
		}
	}
}

// Expects a location object from ConfigParser
void ServerConfig::addLocation(const Location &location, double lineNumber)
{
	if (std::find(this->_locations.begin(), this->_locations.end(), location) != this->_locations.end())
	{
		ConfigUtils::throwConfigError("Duplicate location detected: " + location.getPath() + " at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
	}
	else
		_locations.push_back(location);
}

void ServerConfig::addIndexes(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream indexesStream(line);
	ConfigUtils::validateDirective(indexesStream, "index", lineNumber, __FILE__, __LINE__);

	std::string token;
	while (indexesStream >> token)
	{
		if (token.empty())
		{
			ConfigUtils::throwConfigError("Empty index name at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
		}
		_indexes.push_back(token);
	}
}

void ServerConfig::addStatusPages(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream errorPagesStream(line);
	ConfigUtils::validateDirective(errorPagesStream, "error_page", lineNumber, __FILE__, __LINE__);

	std::string token;
	std::vector<int> errorCodes;
	std::string filePath;

	// Parse tokens: collect error codes first, then file path
	while (errorPagesStream >> token)
	{
		// Try to parse as error code
		char *endPtr;
		long code = std::strtol(token.c_str(), &endPtr, 10);

		if (*endPtr == '\0' && code >= 300 && code <= 599)
		{
			// Valid error code
			errorCodes.push_back(static_cast<int>(code));
		}
		else
		{
			// This should be the file path (last token)
			if (!filePath.empty())
			{
				ConfigUtils::throwConfigError("Invalid error_page directive at line " + StringUtils::toString(lineNumber) + ": Multiple file paths specified", __FILE__, __LINE__);
			}
			filePath = token;
		}
	}

	// Validation
	if (errorCodes.empty())
	{
		ConfigUtils::throwConfigError("Invalid error_page directive at line " + StringUtils::toString(lineNumber) + ": No error codes specified", __FILE__, __LINE__);
	}

	if (filePath.empty())
	{
		ConfigUtils::throwConfigError("Invalid error_page directive at line " + StringUtils::toString(lineNumber) + ": No file path specified", __FILE__, __LINE__);
	}
	// Store the error pages
	for (std::vector<int>::const_iterator it = errorCodes.begin(); it != errorCodes.end(); ++it)
	{
		_statusPages[*it] = filePath;
	}
}

void ServerConfig::addRoot(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream rootStream(line);
	ConfigUtils::validateDirective(rootStream, "root", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(rootStream >> token) || token.empty())
	{
		ConfigUtils::throwConfigError("Empty root path at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
	}

	_root = token;
}

void ServerConfig::addMaxUriSize(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);
	std::stringstream sizeStream(line);
	ConfigUtils::validateDirective(sizeStream, "max_uri_size", lineNumber, __FILE__, __LINE__);
	_maxUriSize = ConfigUtils::parseSizeValue(sizeStream, "max_uri_size", lineNumber);
}

void ServerConfig::addMaxHeaderSize(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);
	std::stringstream sizeStream(line);
	ConfigUtils::validateDirective(sizeStream, "max_header_size", lineNumber, __FILE__, __LINE__);
	_maxHeaderSize = ConfigUtils::parseSizeValue(sizeStream, "max_header_size", lineNumber);
}

void ServerConfig::addClientMaxBodySize(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);
	std::stringstream sizeStream(line);
	ConfigUtils::validateDirective(sizeStream, "client_max_body_size", lineNumber, __FILE__, __LINE__);
	_clientMaxBodySize = ConfigUtils::parseSizeValue(sizeStream, "client_max_body_size", lineNumber);
}

void ServerConfig::addAutoindex(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);

	std::stringstream autoindexStream(line);
	ConfigUtils::validateDirective(autoindexStream, "autoindex", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(autoindexStream >> token) || (token != "on" && token != "off"))
	{
		ConfigUtils::throwConfigError("Invalid autoindex value at line " + StringUtils::toString(lineNumber) + ": " + token, __FILE__, __LINE__);
	}
	_autoindex = (token == "on");
}

void ServerConfig::addAccessLog(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);
	std::stringstream accessLogStream(line);
	ConfigUtils::validateDirective(accessLogStream, "access_log", lineNumber, __FILE__, __LINE__);

	std::string logPath;
	if (!(accessLogStream >> logPath))
	{
		ConfigUtils::throwConfigError("Missing log path for access_log at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
	}
	_accessLog = logPath;
}

void ServerConfig::addErrorLog(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);
	std::stringstream errorLogStream(line);
	ConfigUtils::validateDirective(errorLogStream, "error_log", lineNumber, __FILE__, __LINE__);

	std::string logPath;
	if (!(errorLogStream >> logPath))
	{
		ConfigUtils::throwConfigError("Missing log path for error_log at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
	}
	_errorLog = logPath;
}

void ServerConfig::addKeepAlive(std::string line, double lineNumber)
{
	ConfigUtils::lineValidation(line, lineNumber);
	std::stringstream keepAliveStream(line);
	ConfigUtils::validateDirective(keepAliveStream, "keep_alive", lineNumber, __FILE__, __LINE__);

	std::string token;
	if (!(keepAliveStream >> token) || (token != "on" && token != "off"))
	{
		ConfigUtils::throwConfigError("Invalid keep_alive value at line " + StringUtils::toString(lineNumber) + ": " + token, __FILE__, __LINE__);
	}
	_keepAlive = (token == "on");
}

// Debugging methods
void ServerConfig::printConfig() const
{
	std::cout << "Server Configuration:" << std::endl;

	if (!_hosts_ports.empty())
	{
		std::cout << "Listen Directives:" << std::endl;
		for (std::vector<std::pair<std::string, unsigned short> >::const_iterator it = _hosts_ports.begin();
			 it != _hosts_ports.end(); ++it)
		{
			std::cout << "  " << (it->first.empty() ? "*" : it->first) << ":" << it->second << std::endl;
		}
	}
	std::cout << "Root: " << _root << std::endl;
	std::cout << "Index: ";
	for (size_t i = 0; i < _indexes.size(); ++i)
	{
		std::cout << _indexes[i] << " ";
	}
	std::cout << std::endl;
	std::cout << "Max URI Size: " << _maxUriSize << " bytes" << std::endl;
	std::cout << "Max Header Size: " << _maxHeaderSize << " bytes" << std::endl;
	std::cout << "Client Max Body Size: " << _clientMaxBodySize << " bytes" << std::endl;
	std::cout << "Autoindex: " << (_autoindex ? "on" : "off") << std::endl;

	if (!_serverNames.empty())
	{
		std::cout << "Server Names: ";
		for (std::vector<std::string>::const_iterator it = _serverNames.begin(); it != _serverNames.end(); ++it)
		{
			if (it != _serverNames.begin())
				std::cout << ", ";
			std::cout << *it;
		}
		std::cout << std::endl;
	}

	if (!_statusPages.empty())
	{
		std::cout << "Error Pages:" << std::endl;
		for (std::map<int, std::string>::const_iterator it = _statusPages.begin(); it != _statusPages.end(); ++it)
		{
			std::cout << "  " << it->first << " -> " << it->second << std::endl;
		}
	}

	if (!_locations.empty())
	{
		std::cout << "  Locations:" << std::endl;
		for (std::vector<Location>::const_iterator it = _locations.begin(); it != _locations.end(); ++it)
		{
			it->printConfig();
		}
	}
	if (!_accessLog.empty())
	{
		std::cout << "Access Log: " << _accessLog << std::endl;
	}
	if (!_errorLog.empty())
	{
		std::cout << "Error Log: " << _errorLog << std::endl;
	}
	std::cout << "Keep Alive: " << (_keepAlive ? "on" : "off") << std::endl;
}

// Utils removed - now use ConfigUtils

bool ServerConfig::try_validate_port_only(std::string token)
{
	struct addrinfo hints, *result;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV; // Force numeric port
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo("0.0.0.0", token.c_str(), &hints, &result);
	if (status == 0)
	{
		freeaddrinfo(result);
		return true;
	}
	return false;
}

bool ServerConfig::try_validate_host_only(std::string token)
{
	struct addrinfo hints, *result;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(token.c_str(), "80", &hints, &result); // Use dummy port
	if (status == 0)
	{
		freeaddrinfo(result);
		return true;
	}
	return false;
}

std::pair<std::string, unsigned short> ServerConfig::split_host_port(std::string token)
{
	// Handle IPv6: [::1]:8080
	if (token.find_first_of('[') == 0)
	{
		size_t bracket_end = token.find_last_of(']');
		if (bracket_end != std::string::npos && bracket_end + 1 < token.length() && token[bracket_end + 1] == ':')
		{
			std::string host = token.substr(1, bracket_end - 1);
			std::string port = token.substr(bracket_end + 2);
			if (try_validate_host_only(host) && try_validate_port_only(port))
			{
				return std::make_pair(host, static_cast<unsigned short>(std::strtol(port.c_str(), NULL, 10)));
			}
			else
				return std::make_pair("", 0);
		}
	}

	// Handle IPv4: 192.168.1.100:8080
	size_t colon_pos = token.find_last_of(':'); // Last colon (in case IPv6 without brackets)
	if (colon_pos != std::string::npos)
	{
		std::string host = token.substr(0, colon_pos);
		std::string port = token.substr(colon_pos + 1);
		if (try_validate_host_only(host) && try_validate_port_only(port))
		{
			return std::make_pair(host, static_cast<unsigned short>(std::strtol(port.c_str(), NULL, 10)));
		}
		else
			return std::make_pair("", 0);
	}

	return std::make_pair("", 0);
}

bool ServerConfig::hasLocation(const Location &location) const
{
	return std::find(_locations.begin(), _locations.end(), location) != _locations.end();
}
