#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>


#include "LocationConfig.hpp"

struct ListenDirective {
    std::string host; // Can be IP or empty (for default)
    int port;

    ListenDirective(const std::string& h, int p) : host(h), port(p) {}

    // Equality and ordering for use in sets/maps
    // bool operator<(const ListenDirective& other) const {
    //     return std::tie(host, port) < std::tie(other.host, other.port);
    // }
};

class ServerConfig {
    private:
        std::vector<ListenDirective> _listenDirectives;
        std::string _root;
        std::vector<std::string> _index;
        unsigned long _clientMaxBodySize;
        std::vector<std::string> _serverNames;
        std::map<int, std::string> _errorPages;
        std::vector<LocationConfig> _locations;
        std::string _accessLog;
        std::string _errorLog;

    public:
        ServerConfig();
        ~ServerConfig();

        const std::string& getHost() const;
        const std::vector<int>& getPorts() const;
        const std::vector<ListenDirective>& getListenDirectives() const;
        const std::string& getRoot() const;
        const std::vector<std::string>& getIndex() const;
        unsigned long getClientMaxBodySize() const;
        const std::vector<std::string>& getServerNames() const;
        const std::map<int, std::string>& getErrorPages() const;
        const std::vector<LocationConfig>& getLocations() const;
        const std::string& getAccessLog() const;
        const std::string& getErrorLog() const;

        void setHost(const std::string& host);
        void addPort(int port);
        void addListenDirective(const std::string& host, int port);
        void setRoot(const std::string& root);
        void setIndex(const std::vector<std::string>& index);
        void setClientMaxBodySize(unsigned long size);
        void addServerName(const std::string& serverName);
        void setServerNames(const std::vector<std::string>& serverNames);
        void addErrorPage(int errorCode, const std::string& errorPage);
        void addErrorPages(const std::vector<int>& errorCodes, const std::string& errorPage);
        void addLocation(const LocationConfig& location);
        void setSslCertificate(const std::string& certPath);
        void setSslCertificateKey(const std::string& keyPath);
        void addSslProtocol(const std::string& protocol);
        void setAccessLog(const std::string& logPath);
        void setErrorLog(const std::string& logPath);

        void printConfig() const;
};

// Validation function for server name and port uniqueness
void validateServerNamePortUniqueness(const std::vector<ServerConfig>& servers);

#endif