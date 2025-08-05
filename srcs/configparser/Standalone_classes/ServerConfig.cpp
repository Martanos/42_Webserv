#include "srcs/configparser/Standalone_classes/ServerConfig.hpp"

// Constructors
ServerConfig::ServerConfig()
{
    _clientMaxBodySize = 1048576; // default value of 1MB
}

// Destructor
ServerConfig::~ServerConfig() {}

// Getters
const std::vector<std::string> &ServerConfig::getServerNames() const { return _serverNames; }
const std::vector<std::pair<std::string, unsigned short> > &ServerConfig::getHosts_ports() const { return _hosts_ports; }
const std::string &ServerConfig::getRoot() const { return _root; }
const std::vector<std::string> &ServerConfig::getIndexes() const { return _indexes; }
bool ServerConfig::getAutoindex() const { return _autoindex; }
double ServerConfig::getClientMaxBodySize() const { return _clientMaxBodySize; }
const std::map<int, std::string> &ServerConfig::getErrorPages() const { return _errorPages; }
const std::vector<LocationConfig> &ServerConfig::getLocations() const { return _locations; }
const std::string &ServerConfig::getAccessLog() const { return _accessLog; }
const std::string &ServerConfig::getErrorLog() const { return _errorLog; }

// Setters
void ServerConfig::setServerNames(const std::vector<std::string> &serverNames) { _serverNames = serverNames; }
void ServerConfig::setHosts_ports(const std::vector<std::pair<std::string, unsigned short> > &hosts_ports) { _hosts_ports = hosts_ports; }
void ServerConfig::setRoot(const std::string &root) { _root = root; }
void ServerConfig::setIndexes(const std::vector<std::string> &indexes) { _indexes = indexes; }
void ServerConfig::setAutoindex(bool autoindex) { _autoindex = autoindex; }
void ServerConfig::setClientMaxBodySize(double size) { _clientMaxBodySize = size; }
void ServerConfig::setResponsePages(const std::map<int, std::string> &responsePages) { _responsePages = responsePages; }
void ServerConfig::setLocations(const std::vector<LocationConfig> &locations) { _locations = locations; }
void ServerConfig::setAccessLog(const std::string &accessLog) { _accessLog = accessLog; }
void ServerConfig::setErrorLog(const std::string &errorLog) { _errorLog = errorLog; }

// Parsing methods
void ServerConfig::addServerName(std::string line, double lineNumber)
{
    lineValidation(line, lineNumber);

    std::stringstream serverNameStream(line);
    std::string token;
    std::stringstream errorMessage;

    if (!(serverNameStream >> token) || token != "server_name")
    {
        errorMessage << "Invalid server_name directive at line " << lineNumber;
        Logger::log(Logger::ERROR, errorMessage.str());
        throw std::runtime_error(errorMessage.str());
    }
    // Server name validation and vector population
    while (serverNameStream >> token)
    {
        // Special cases
        if (token == "_") // catch all
        {
            if (std::find(this->_serverNames.begin(), this->_serverNames.end(), token) != this->_serverNames.end())
            {
                errorMessage << "Duplicate catch-all server_name detected: " << token << " at line " << lineNumber;
                Logger::log(Logger::ERROR, errorMessage.str());
                throw std::runtime_error(errorMessage.str());
            }
            else
                _serverNames.push_back(token);
            continue;
        }
        // Simple verification if hostname is valid (no special characters)
        if (token.length() > 255)
        {
            errorMessage << "Invalid server_name at line " << lineNumber << ": " << token << " (Hostname exceeds 255 characters)";
            Logger::log(Logger::ERROR, errorMessage.str());
            throw std::runtime_error(errorMessage.str());
        }
        else if (token.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_") != std::string::npos)
        {
            errorMessage << "Invalid server_name at line " << lineNumber << ": " << token << " (Contains invalid characters)";
            Logger::log(Logger::ERROR, errorMessage.str());
            throw std::runtime_error(errorMessage.str());
        }
        else if (!isalpha(token[0]) || !isalnum(token[token.length() - 1]))
        {
            errorMessage << "Invalid server_name at line " << lineNumber << ": " << token << " (Hostname must begin and end with alphanumeric character)";
            Logger::log(Logger::ERROR, errorMessage.str());
            throw std::runtime_error(errorMessage.str());
        }
        else if (std::find(this->_serverNames.begin(), this->_serverNames.end(), token) != this->_serverNames.end())
        {
            errorMessage << "Duplicate server_name detected: " << token << " at line " << lineNumber;
            Logger::log(Logger::ERROR, errorMessage.str());
            throw std::runtime_error(errorMessage.str());
        }
        else
            _serverNames.push_back(token);

        if (_serverNames.size() >= 255)
        {
            errorMessage << "Too many server_names at line " << lineNumber << " (Maximum of 255 server_names allowed)";
            Logger::log(Logger::ERROR, errorMessage.str());
            throw std::runtime_error(errorMessage.str());
        }
    }
}

//
void ServerConfig::addHosts_ports(std::string line, double lineNumber)
{
    lineValidation(line, lineNumber);

    std::stringstream hosts_portsStream(line);
    std::string token;
    std::stringstream errorMessage;

    if (!(hosts_portsStream >> token) || token != "listen")
    {
        errorMessage << "Invalid listen directive at line " << lineNumber;
        Logger::log(Logger::ERROR, errorMessage.str());
        throw std::runtime_error(errorMessage.str());
    }

    while (hosts_portsStream >> token)
    {
        std::pair<std::string, unsigned short> host_port;
        if (try_validate_port_only(token))
        {
            host_port = std::make_pair("0.0.0.0", std::strtol(token.c_str(), NULL, 10));
            if (std::find(this->_hosts_ports.begin(), this->_hosts_ports.end(), host_port) != this->_hosts_ports.end())
            {
                errorMessage << "Duplicate listen directive detected: " << token << " at line " << lineNumber;
                Logger::log(Logger::ERROR, errorMessage.str());
                throw std::runtime_error(errorMessage.str());
            }
            else
                _hosts_ports.push_back(host_port);
            continue;
        }
        else if (try_validate_host_only(token))
        {
            host_port = std::make_pair(token, 80);
            if (std::find(this->_hosts_ports.begin(), this->_hosts_ports.end(), host_port) != this->_hosts_ports.end())
            {
                errorMessage << "Duplicate listen directive detected: " << token << " at line " << lineNumber;
                Logger::log(Logger::ERROR, errorMessage.str());
                throw std::runtime_error(errorMessage.str());
            }
            else
                _hosts_ports.push_back(host_port);
            continue;
        }
        // Try to split into host:port
        std::pair<std::string, unsigned short> host_port = split_host_port(token);
        if (host_port.first.empty() && host_port.second == 0)
        {
            errorMessage << "Invalid listen directive: " << token << " at line " << lineNumber;
            Logger::log(Logger::ERROR, errorMessage.str());
            throw std::runtime_error(errorMessage.str());
        }
        else
        {
            if (std::find(this->_hosts_ports.begin(), this->_hosts_ports.end(), host_port) != this->_hosts_ports.end())
            {
                errorMessage << "Duplicate listen directive detected: " << token << " at line " << lineNumber;
                Logger::log(Logger::ERROR, errorMessage.str());
                throw std::runtime_error(errorMessage.str());
            }
            else
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
        Logger::log(Logger::ERROR, errorMessage.str());
        throw std::runtime_error(errorMessage.str());
    }
    else
        _locations.push_back(location);
}

void ServerConfig::addResponsePages(std::string line, double lineNumber)
{
    lineValidation(line, lineNumber);

    std::stringstream errorPagesStream(line);
    std::string token;
    std::stringstream errorMessage;

    if (!(errorPagesStream >> token) || token != "response_page")
    {
        errorMessage << "Invalid response_page directive at line " << lineNumber;
        Logger::log(Logger::ERROR, errorMessage.str());
        throw std::runtime_error(errorMessage.str());
    }

    // TODO: Move response code validation here
    while (errorPagesStream >> token)
    {
        double responseCode = std::strtod(token.c_str(), NULL);
        if (responseCode < 100 || responseCode > 600)
        {
            errorMessage << "Invalid response_page directive at line " << lineNumber << ": " << token << " (Invalid response code)";
            Logger::log(Logger::ERROR, errorMessage.str());
            throw std::runtime_error(errorMessage.str());
        }
    }
}

// TODO: Move access log validation here
void ServerConfig::addAccessLog(std::string line, double lineNumber)
{
    lineValidation(line, lineNumber);
    std::stringstream accessLogStream(line);
    std::string token;
    std::stringstream errorMessage;

    if (!(accessLogStream >> token) || token != "access_log")
    {
        errorMessage << "Invalid access_log directive at line " << lineNumber;
        Logger::log(Logger::ERROR, errorMessage.str());
        throw std::runtime_error(errorMessage.str());
    }

    while (accessLogStream >> token)
    {
        if (token == "access_log")
            continue;
    }
}

// TODO: Move error log validation here
void ServerConfig::addErrorLog(std::string line, double lineNumber)
{
    lineValidation(line, lineNumber);
    std::stringstream errorLogStream(line);
    std::string token;
    std::stringstream errorMessage;

    if (!(errorLogStream >> token) || token != "error_log")
    {
        errorMessage << "Invalid error_log directive at line " << lineNumber;
        Logger::log(Logger::ERROR, errorMessage.str());
        throw std::runtime_error(errorMessage.str());
    }

    while (errorLogStream >> token)
    {
        if (token == "error_log")
            continue;
    }
}

// Debugging methods
void ServerConfig::printConfig() const
{
    std::cout << "Server Configuration:" << std::endl;

    if (!_hosts_ports.empty())
    {
        std::cout << "Listen Directives:" << std::endl;
        for (std::vector<std::pair<std::string, unsigned short> >::const_iterator it = _hosts_ports.begin(); it != _hosts_ports.end(); ++it)
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

    if (!_errorPages.empty())
    {
        std::cout << "Error Pages:" << std::endl;
        for (std::map<int, std::string>::const_iterator it = _errorPages.begin(); it != _errorPages.end(); ++it)
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
}

// Utils

void ServerConfig::lineValidation(std::string line, int lineNumber)
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
    line.erase(line.find_last_of(';') + 1);
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
                return {host, std::strtol(port.c_str(), NULL, 10)};
            }
            else
                return;
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
            return {host, std::strtol(port.c_str(), NULL, 10)};
        }
        else
            return;
    }

    return;
}
