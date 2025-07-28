#include "ServerConfig.hpp"


ServerConfig::ServerConfig() : _host("localhost"), _root("/var/www"), _clientMaxBodySize(1048576),
_accessLog(""), _errorLog("") {
    _ports.clear();
    _serverNames.clear();
    _errorPages.clear();
    _index.clear();
}

ServerConfig::~ServerConfig() {}

//getters
const std::string& ServerConfig::getHost() const { return _host; }
const std::vector<int>& ServerConfig::getPorts() const { return _ports; }
const std::string& ServerConfig::getRoot() const { return _root; }
const std::vector<std::string>& ServerConfig::getIndex() const { return _index; }
unsigned long ServerConfig::getClientMaxBodySize() const { return _clientMaxBodySize; }
const std::vector<std::string>& ServerConfig::getServerNames() const { return _serverNames; }
const std::map<int, std::string>& ServerConfig::getErrorPages() const { return _errorPages; }
const std::vector<LocationConfig>& ServerConfig::getLocations() const { return _locations; }
const std::string& ServerConfig::getAccessLog() const { return _accessLog; }
const std::string& ServerConfig::getErrorLog() const { return _errorLog; }


//setters
void ServerConfig::setHost(const std::string& host) { _host = host; }
void ServerConfig::setRoot(const std::string& root) { _root = root; }
void ServerConfig::setIndex(const std::vector<std::string>& index) { _index = index; }
void ServerConfig::setClientMaxBodySize(unsigned long size) { _clientMaxBodySize = size; }
void ServerConfig::addPort(int port) {
    if (port > 0 && port < 65536) { // Valid port range
        _ports.push_back(port);
    } else {
        std::cerr << "Invalid port number: " << port << ". Port must be between 1 and 65535." << std::endl;
    }
}
void ServerConfig::addLocation(const LocationConfig& location) { _locations.push_back(location); }
void ServerConfig::addServerName(const std::string& serverName) { _serverNames.push_back(serverName); }
void ServerConfig::setServerNames(const std::vector<std::string>& serverNames) { _serverNames = serverNames; }
void ServerConfig::addErrorPage(int errorCode, const std::string& errorPage) { _errorPages[errorCode] = errorPage; }
void ServerConfig::addErrorPages(const std::vector<int>& errorCodes, const std::string& errorPage) {
    for (std::vector<int>::const_iterator it = errorCodes.begin(); it != errorCodes.end(); ++it) {
        _errorPages[*it] = errorPage;
    }
}
void ServerConfig::setAccessLog(const std::string& logPath) { _accessLog = logPath; }
void ServerConfig::setErrorLog(const std::string& logPath) { _errorLog = logPath; }


void ServerConfig::printConfig() const {
    std::cout << "Server Configuration:" << std::endl;
    std::cout << "Host: " << _host << std::endl;
    std::cout << "Ports: ";
    for (size_t i = 0; i < _ports.size(); ++i) {
        std::cout << _ports[i] << (i == _ports.size() - 1 ? "" : ", ");
    }
    std::cout << std::endl;
    std::cout << "Root: " << _root << std::endl;
    std::cout << "Index: ";
    for (size_t i = 0; i < _index.size(); ++i) {
        std::cout << _index[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Client Max Body Size: " << _clientMaxBodySize << " bytes" << std::endl;
    
    if (!_serverNames.empty()) {
        std::cout << "Server Names: ";
        for (std::vector<std::string>::const_iterator it = _serverNames.begin(); it != _serverNames.end(); ++it) {
            if (it != _serverNames.begin()) std::cout << ", ";
            std::cout << *it;
        }
        std::cout << std::endl;
    }
    
    if (!_errorPages.empty()) {
        std::cout << "Error Pages:" << std::endl;
        for (std::map<int, std::string>::const_iterator it = _errorPages.begin(); it != _errorPages.end(); ++it) {
            std::cout << "  " << it->first << " -> " << it->second << std::endl;
        }
    }

    if (!_locations.empty()) {
        std::cout << "  Locations:" << std::endl;
        for (std::vector<LocationConfig>::const_iterator it = _locations.begin(); it != _locations.end(); ++it) {
            it->printConfig();
        }
    }
    if (!_accessLog.empty()) {
        std::cout << "Access Log: " << _accessLog << std::endl;
    }
    if (!_errorLog.empty()) {
        std::cout << "Error Log: " << _errorLog << std::endl;
    }
}
