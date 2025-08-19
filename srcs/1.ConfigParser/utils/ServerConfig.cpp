#include "ServerConfig.hpp"
#include <set>
#include <stdexcept>
#include <string>
#include <sstream>


ServerConfig::ServerConfig() : _root("/var/www"), _clientMaxBodySize(1048576),
_accessLog(""), _errorLog("") {
    _listenDirectives.clear();
    _serverNames.clear();
    _errorPages.clear();
    _index.clear();
}

ServerConfig::~ServerConfig() {}

//getters
const std::string& ServerConfig::getHost() const { 
    static std::string defaultHost = "localhost";
    if (!_listenDirectives.empty() && !_listenDirectives[0].host.empty()) {
        return _listenDirectives[0].host;
    }
    return defaultHost;
}
const std::vector<int>& ServerConfig::getPorts() const { 
    static std::vector<int> ports;
    ports.clear();
    for (std::vector<ListenDirective>::const_iterator it = _listenDirectives.begin(); it != _listenDirectives.end(); ++it) {
        ports.push_back(it->port);
    }
    return ports;
}
const std::vector<ListenDirective>& ServerConfig::getListenDirectives() const { return _listenDirectives; }
const std::string& ServerConfig::getRoot() const { return _root; }
const std::vector<std::string>& ServerConfig::getIndex() const { return _index; }
unsigned long ServerConfig::getClientMaxBodySize() const { return _clientMaxBodySize; }
const std::vector<std::string>& ServerConfig::getServerNames() const { return _serverNames; }
const std::map<int, std::string>& ServerConfig::getErrorPages() const { return _errorPages; }
const std::vector<LocationConfig>& ServerConfig::getLocations() const { return _locations; }
const std::string& ServerConfig::getAccessLog() const { return _accessLog; }
const std::string& ServerConfig::getErrorLog() const { return _errorLog; }


//setters
void ServerConfig::setHost(const std::string& host) { 
    if (!_listenDirectives.empty()) {
        _listenDirectives[0].host = host;
    } else {
        _listenDirectives.push_back(ListenDirective(host, 80)); // default port
    }
}
void ServerConfig::setRoot(const std::string& root) { _root = root; }
void ServerConfig::setIndex(const std::vector<std::string>& index) { _index = index; }
void ServerConfig::setClientMaxBodySize(unsigned long size) { _clientMaxBodySize = size; }
void ServerConfig::addPort(int port) {
    _listenDirectives.push_back(ListenDirective("", port)); // empty host means default
}
void ServerConfig::addListenDirective(const std::string& host, int port) {
    _listenDirectives.push_back(ListenDirective(host, port));
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
    
    if (!_listenDirectives.empty()) {
        std::cout << "Listen Directives:" << std::endl;
        for (std::vector<ListenDirective>::const_iterator it = _listenDirectives.begin(); it != _listenDirectives.end(); ++it) {
            std::cout << "  " << (it->host.empty() ? "*" : it->host) << ":" << it->port << std::endl;
        }
    }
    
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

// Validation function for server name and port uniqueness
void validateServerNamePortUniqueness(const std::vector<ServerConfig>& servers) {
    std::set<std::pair<std::string, int> > seen;

    for (std::vector<ServerConfig>::const_iterator serverIt = servers.begin(); serverIt != servers.end(); ++serverIt) {
        const std::vector<std::string>& serverNames = serverIt->getServerNames();
        for (std::vector<std::string>::const_iterator nameIt = serverNames.begin(); nameIt != serverNames.end(); ++nameIt) {
            const std::vector<ListenDirective>& listenDirectives = serverIt->getListenDirectives();
            for (std::vector<ListenDirective>::const_iterator listenIt = listenDirectives.begin(); listenIt != listenDirectives.end(); ++listenIt) {
                std::pair<std::string, int> key(*nameIt, listenIt->port);

                if (!seen.insert(key).second) {
                    std::stringstream ss;
                    ss << listenIt->port;
                    throw std::runtime_error("Duplicate server_name:port detected: " + *nameIt + ":" + ss.str());
                }
            }
        }
    }
}
