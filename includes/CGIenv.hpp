#ifndef CGIENV_HPP
#define CGIENV_HPP



#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

#include <string>
#include <map>
#include <iostream>

class CGIenv {
    public:
        CGIenv();
        ~CGIenv();

        void setEnv(const std::string& key, const std::string& value);
        std::string getEnv(const std::string& key) const;
        void printEnv() const;
        void copyDataFromServer(const ServerConfig &server, const LocationConfig &location);

    private:
        
        CGIenv(const CGIenv &other);
        CGIenv& operator=(const CGIenv &other);

        
        std::map<std::string, std::string> _envVariables;
};



#endif