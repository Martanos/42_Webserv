#ifndef CGIENV_HPP
#define CGIENV_HPP

#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

#include <string>
#include <map>
#include <iostream>
#include <cstring>

// Forward declarations
class HttpRequest;
class Server;
class Location;

class CGIenv {
    public:
        CGIenv();
        CGIenv(const CGIenv &other);
        ~CGIenv();

        CGIenv& operator=(const CGIenv &other);

        void setEnv(const std::string& key, const std::string& value);
        std::string getEnv(const std::string& key) const;
        void printEnv() const;
        void copyDataFromServer(const ServerConfig &server, const LocationConfig &location);
        
        // New methods for CGI execution
        void setupFromRequest(const HttpRequest &request, 
                             const Server *server, 
                             const Location *location,
                             const std::string &scriptPath);
        void setupHttpHeaders(const HttpRequest &request);
        std::string convertHeaderNameToCgi(const std::string &headerName) const;
        
        // Environment array management for execve
        char **getEnvArray() const;
        void freeEnvArray(char **envArray) const;
        
        // Utility methods
        size_t getEnvCount() const;
        bool hasEnv(const std::string &key) const;
        void removeEnv(const std::string &key);
        void clear();

    private:
        std::map<std::string, std::string> _envVariables;
};

#endif
