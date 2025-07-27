#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>


#include "LocationConfig.hpp"

class ServerConfig {
    private:
        std::string _host;
        std::vector<int> _ports;
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

#endif