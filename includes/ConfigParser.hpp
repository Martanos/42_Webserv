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
            SERVER_ACCESS_LOG,
            SERVER_ERROR_LOG,
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
            LOCATION_TRY_FILES,
            LOCATION_UNKNOWN
        };

        std::vector<ServerConfig> _serverConfigs;
        std::string _trim(const std::string &str) const;
        std::vector<std::string> _split(const std::string &str) const;
        bool _isValidPort(const std::string &portstr) const;
        bool _isValidErrorCode(const std::string &errorCodeStr) const;
        
        // Helper methods for directive type identification
        ServerDirectiveType _getServerDirectiveType(const std::string& directive) const;
        LocationDirectiveType _getLocationDirectiveType(const std::string& directive) const;
        void _parseListenDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) const;
        void _parseRootDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) const;
        void _parseServerNameDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) const;
        void _parseErrorPageDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) const;
        void _parseIndexDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer);
        void _parseClientMaxBodySizeDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer);
        void _parseAccessLogDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer);
        void _parseErrorLogDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer);
        void _parseServerBlock(std::ifstream& file);
        void _parseLocationBlock(std::ifstream& file, ServerConfig& currentServer, const std::string& path);
        bool _getNextDirective(std::ifstream& file, std::vector<std::string>& tokens);
        void _parseLocationRootDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation);
        void _parseAcceptedHttpMethodsDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation);
        void _parseReturnDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation);
        void _parseAutoIndexDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation);
        void _parseIndexDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation);
        
        void _parseFastCgiPassDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation);
        void _parseFastCgiParamDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation);
        void _parseFastCgiIndexDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation);
        void _parseUploadPathDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation);
        void _parseTryFilesDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation);

        ConfigParser& operator=(const ConfigParser &other);
    public:
        ConfigParser();
        ConfigParser (const std::string &filename);
        ~ConfigParser();

        bool parseConfig(const std::string &filename);

        const std::vector<ServerConfig>& getServerConfigs() const;

        void printAllConfigs() const;
};

#endif