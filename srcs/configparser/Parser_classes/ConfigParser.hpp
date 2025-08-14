#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP


#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cctype>
#include <fstream>

#include "ServerConfig.hpp"
#include "Logger.hpp"

class ConfigParser {
    private:
        // Enums for directive types to avoid if-else forests
        enum ServerDirectiveType {
            SERVER_LISTEN,
            SERVER_ROOT,
            SERVER_NAME,
            SERVER_ERROR_PAGE,
            SERVER_INDEX,
            SERVER_CLIENT_MAX_BODY_SIZE,
            SERVER_AUTOINDEX,
            SERVER_ACCESS_LOG,
            SERVER_ERROR_LOG,
            SERVER_LOCATION,
            SERVER_UNKNOWN
        };

        enum LocationDirectiveType {
            LOCATION_ROOT,
            LOCATION_ACCEPTED_HTTP_METHODS,
            LOCATION_RETURN,
            LOCATION_AUTOINDEX,
            LOCATION_INDEX,
            LOCATION_FASTCGI_PASS,
            LOCATION_FASTCGI_PARAM,
            LOCATION_FASTCGI_INDEX,
            LOCATION_UPLOAD_PATH,
            LOCATION_UNKNOWN
        };

        ConfigParser& operator=(const ConfigParser &other);
        ConfigParser(const ConfigParser &other);

        std::vector<ServerConfig> _serverConfigs;
        std::string _trim(const std::string &str) const;
        std::vector<std::string> _split(const std::string &str) const;
        bool serverblockcheck(const std::string &line, bool &insideHttp, bool &insideServer);
        bool httpblockcheck(const std::string &line, bool &foundHttp, bool &insideHttp);
        void _parseServerBlock(std::stringstream &buffer, double &lineNumber);
        void _parseLocationBlock(std::stringstream &buffer, double &lineNumber, const std::string &locationLine, ServerConfig &currentServer);
        ServerDirectiveType _getServerDirectiveType(const std::string &directive) const;
        LocationDirectiveType _getLocationDirectiveType(const std::string &directive) const;

    public:
        ConfigParser();
        ConfigParser(const std::string& filename);
        ~ConfigParser();

        bool parseConfig(const std::string &filename);
        void printAllConfigs() const;
    
};


#endif
