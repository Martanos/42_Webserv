/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: malee <malee@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 18:25:58 by malee             #+#    #+#             */
/*   Updated: 2025/10/21 15:46:43 by malee            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/ServerConfig.hpp"
#include "../../includes/Constants.hpp"

// Constructors
ServerConfig::ServerConfig()
{
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
double ServerConfig::getClientMaxBodySize() const
{
	return _clientMaxBodySize;
}
const std::map<int, std::string> &ServerConfig::getStatusPages() const
{
	return _statusPages;
}
const std::vector<LocationConfig> &ServerConfig::getLocations() const
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

// Setters
// void ServerConfig::setServerNames(const std::vector<std::string>
// &serverNames) { _serverNames = serverNames; } void
// ServerConfig::setHosts_ports(const std::vector<std::pair<std::string,
// unsigned short> > &hosts_ports) { _hosts_ports = hosts_ports; } void
// ServerConfig::setRoot(const std::string &root) { _root = root; } void
// ServerConfig::setIndexes(const std::vector<std::string> &indexes) { _indexes
// = indexes; } void ServerConfig::setAutoindex(bool autoindex) { _autoindex =
// autoindex; } void ServerConfig::setClientMaxBodySize(double size) {
// _clientMaxBodySize = size; } void ServerConfig::setResponsePages(const
// std::map<int, std::string> &responsePages) { _responsePages = responsePages;
// } void ServerConfig::setLocations(const std::vector<LocationConfig>
// &locations) { _locations = locations; } void ServerConfig::setAccessLog(const
// std::string &accessLog) { _accessLog = accessLog; } void
// ServerConfig::setErrorLog(const std::string &errorLog) { _errorLog =
// errorLog; }

std::string _trim(const std::string &str)
{
	// First, remove inline comments
	std::string cleaned = str;
	size_t commentPos = cleaned.find('#');
	if (commentPos != std::string::npos)
	{
		cleaned = cleaned.substr(0, commentPos);
	}

	size_t first = cleaned.find_first_not_of(" \t\n\r");
	if (first == std::string::npos)
		return ""; // No non-whitespace characters
	size_t last = cleaned.find_last_not_of(" \t\n\r");
	return cleaned.substr(first, (last - first + 1));
}

std::vector<std::string> _split(const std::string &str)
{
	std::vector<std::string> tokens;
	std::istringstream stream(str);
	std::string token;

	while (stream >> token)
	{
		tokens.push_back(token);
	}

	return tokens;
}
// // Parsing methods
// void ServerConfig::addServerName(std::string line, double lineNumber)
// {
// 	lineValidation(line, lineNumber);

// 	std::stringstream serverNameStream(line);
// 	std::string token;
// 	std::stringstream errorMessage;

// 	if (!(serverNameStream >> token) || token != "server_name")
// 	{
// 		errorMessage << "Invalid server_name directive at line " << lineNumber;
// 		Logger::error(errorMessage.str(), __FILE__, __LINE__);
// 		throw std::runtime_error(errorMessage.str());
// 	}
// 	// Server name validation and vector population
// 	while (serverNameStream >> token)
// 	{
// 		// Special cases
// 		if (token == "_") // catch all
// 		{
// 			if (std::find(this->_serverNames.begin(), this->_serverNames.end(), token) != this->_serverNames.end())
// 			{
// 				errorMessage << "Duplicate catch-all server_name detected: " << token << " at line " << lineNumber;
// 				Logger::error(errorMessage.str(), __FILE__, __LINE__);
// 				throw std::runtime_error(errorMessage.str());
// 			}
// 			else
// 				_serverNames.push_back(token);
// 			continue;
// 		}
// 		// Simple verification if hostname is valid (no special characters)
// 		if (token.length() > 255)
// 		{
// 			errorMessage << "Invalid server_name at line " << lineNumber << ": " << token
// 						 << " (Hostname exceeds 255 characters)";
// 			Logger::error(errorMessage.str(), __FILE__, __LINE__);
// 			throw std::runtime_error(errorMessage.str());
// 		}
// 		else if (token.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJK"
// 										 "LMNOPQRSTUVWXYZ0123456789.-_") != std::string::npos)
// 		{
// 			errorMessage << "Invalid server_name at line " << lineNumber << ": " << token
// 						 << " (Contains invalid characters)";
// 			Logger::error(errorMessage.str(), __FILE__, __LINE__);
// 			throw std::runtime_error(errorMessage.str());
// 		}
// 		else if (!isalpha(token[0]) || !isalnum(token[token.length() - 1]))
// 		{
// 			errorMessage << "Invalid server_name at line " << lineNumber << ": " << token
// 						 << " (Hostname must begin and end with alphanumeric character)";
// 			Logger::error(errorMessage.str(), __FILE__, __LINE__);
// 			throw std::runtime_error(errorMessage.str());
// 		}
// 		else if (std::find(this->_serverNames.begin(), this->_serverNames.end(), token) != this->_serverNames.end())
// 		{
// 			errorMessage << "Duplicate server_name detected: " << token << " at line " << lineNumber;
// 			Logger::error(errorMessage.str(), __FILE__, __LINE__);
// 			throw std::runtime_error(errorMessage.str());
// 		}
// 		else
// 			_serverNames.push_back(token);

// 		if (_serverNames.size() >= 255)
// 		{
// 			errorMessage << "Too many server_names at line " << lineNumber << " (Maximum of 255 server_names allowed)";
// 			Logger::error(errorMessage.str(), __FILE__, __LINE__);
// 			throw std::runtime_error(errorMessage.str());
// 		}
// 	}
// }

//
void ServerConfig::addHosts_ports(std::string line, double lineNumber)
{
	Logger::debug("ServerConfig: Processing listen directive: " + line);
	lineValidation(line, lineNumber);

	std::stringstream hosts_portsStream(line);
	std::string token;
	std::stringstream errorMessage;

	// if (!(hosts_portsStream >> token) || token != "listen")
	// {
	// 	errorMessage << "Invalid listen directive at line " << lineNumber;
	// 	Logger::error(errorMessage.str(), __FILE__, __LINE__);
	// 	throw std::runtime_error(errorMessage.str());
	// }

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
				errorMessage << "Invalid host:port format: " << token << " at line " << lineNumber;
				Logger::error(errorMessage.str(), __FILE__, __LINE__);
				throw std::runtime_error(errorMessage.str());
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
			errorMessage << "Invalid listen directive: " << token << " at line " << lineNumber;
			Logger::error(errorMessage.str(), __FILE__, __LINE__);
			throw std::runtime_error(errorMessage.str());
		}

		// Check for duplicates and add to the list
		if (std::find(this->_hosts_ports.begin(), this->_hosts_ports.end(), host_port) != this->_hosts_ports.end())
		{
			errorMessage << "Duplicate listen directive detected: " << token << " at line " << lineNumber;
			Logger::error(errorMessage.str(), __FILE__, __LINE__);
			throw std::runtime_error(errorMessage.str());
		}
		else
		{
			_hosts_ports.push_back(host_port);
		}
	}
}

// Expects a location object from LocationConfig.cpp
void ServerConfig::addLocation(const LocationConfig &location, double lineNumber)
{
	std::stringstream errorMessage;
	if (std::find(this->_locations.begin(), this->_locations.end(), location) != this->_locations.end())
	{
		errorMessage << "Duplicate location detected: " << location.getPath() << " at line " << lineNumber;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}
	else
		_locations.push_back(location);
}

void ServerConfig::addIndexes(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream indexesStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(indexesStream >> token) || token != "index")
	{
		errorMessage << "Invalid index directive at line " << lineNumber;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}

	while (indexesStream >> token)
	{
		if (token.empty())
		{
			errorMessage << "CHECK PLS Empty index name at line " << lineNumber;
			Logger::error(errorMessage.str(), __FILE__, __LINE__);
			throw std::runtime_error(errorMessage.str());
		}
		_indexes.push_back(token);
	}
}

void ServerConfig::addStatusPages(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream errorPagesStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(errorPagesStream >> token) || token != "error_page")
	{
		errorMessage << "Invalid error_page directive at line " << lineNumber;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}

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
				errorMessage << "Invalid error_page directive at line " << lineNumber
							 << ": Multiple file paths specified";
				Logger::error(errorMessage.str(), __FILE__, __LINE__);
				throw std::runtime_error(errorMessage.str());
			}
			filePath = token;
		}
	}

	// Validation
	if (errorCodes.empty())
	{
		errorMessage << "Invalid error_page directive at line " << lineNumber << ": No error codes specified";
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}

	if (filePath.empty())
	{
		errorMessage << "Invalid error_page directive at line " << lineNumber << ": No file path specified";
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}
	// Store the error pages
	for (std::vector<int>::const_iterator it = errorCodes.begin(); it != errorCodes.end(); ++it)
	{
		_statusPages[*it] = filePath;
	}
}

void ServerConfig::addRoot(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream rootStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(rootStream >> token) || token != "root")
	{
		errorMessage << "Invalid root directive at line " << lineNumber;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}

	if (!(rootStream >> token) || token.empty())
	{
		errorMessage << "Empty root path at line " << lineNumber;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}

	_root = token;
}

void ServerConfig::addClientMaxBodySize(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream sizeStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(sizeStream >> token) || token != "client_max_body_size")
	{
		errorMessage << "Invalid client_max_body_size directive at line " << lineNumber;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}
	if (token.empty())
	{
		errorMessage << "CHECK PLS Empty client_max_body_size at line " << lineNumber;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}
	sizeStream >> token; // Read the size value
	char lastChar = token[token.length() - 1];
	token.erase(token.length() - 1);
	std::stringstream ss(token);
	unsigned long size = 0;
	ss >> size;
	if (lastChar == 'k' || lastChar == 'K')
	{
		size *= 1024;
	}
	else if (lastChar == 'm' || lastChar == 'M')
	{
		size *= 1024 * 1024;
	}
	else if (lastChar == 'g' || lastChar == 'G')
	{
		size *= 1024 * 1024 * 1024;
	}
	_clientMaxBodySize = size;
}

void ServerConfig::addAutoindex(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);

	std::stringstream autoindexStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(autoindexStream >> token) || token != "autoindex")
	{
		errorMessage << "Invalid autoindex directive at line " << lineNumber;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}

	if (!(autoindexStream >> token) || (token != "on" && token != "off"))
	{
		errorMessage << "Invalid autoindex value at line " << lineNumber << ": " << token;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}
	_autoindex = (token == "on");
}

void ServerConfig::addAccessLog(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);
	std::stringstream accessLogStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(accessLogStream >> token) || token != "access_log")
	{
		errorMessage << "Invalid access_log directive at line " << lineNumber;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}

	while (accessLogStream >> token)
	{
		if (token == "access_log")
			continue;
	}
}

void ServerConfig::addErrorLog(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);
	std::stringstream errorLogStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(errorLogStream >> token) || token != "error_log")
	{
		errorMessage << "Invalid error_log directive at line " << lineNumber;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}

	while (errorLogStream >> token)
	{
		if (token == "error_log")
			continue;
	}
}

void ServerConfig::addKeepAlive(std::string line, double lineNumber)
{
	lineValidation(line, lineNumber);
	std::stringstream keepAliveStream(line);
	std::string token;
	std::stringstream errorMessage;

	if (!(keepAliveStream >> token) || token != "keep_alive")
	{
		errorMessage << "Invalid keep_alive directive at line " << lineNumber;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}
	if (!(keepAliveStream >> token) || (token != "on" && token != "off"))
	{
		errorMessage << "Invalid keep_alive value at line " << lineNumber << ": " << token;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
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
		for (std::vector<LocationConfig>::const_iterator it = _locations.begin(); it != _locations.end(); ++it)
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

// Utils

void ServerConfig::lineValidation(std::string &line, int lineNumber)
{
	std::stringstream errorMessage;
	if (line.empty())
	{
		errorMessage << "Empty directive at line " << lineNumber;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}
	else if (line.find_last_of(';') != line.length() - 1)
	{
		errorMessage << "Expected semicolon at the end of line " << lineNumber << " : " << line;
		Logger::error(errorMessage.str(), __FILE__, __LINE__);
		throw std::runtime_error(errorMessage.str());
	}
	line.erase(line.find_last_of(';'));
}

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

bool ServerConfig::hasLocation(const LocationConfig &location) const
{
	return std::find(_locations.begin(), _locations.end(), location) != _locations.end();
}
