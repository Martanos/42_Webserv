#include "ConfigParser.hpp"

bool ConfigParser::_isValidPort(const std::string &portStr) const {
    if (portStr.empty()) return false;
    
    for (size_t i = 0; i < portStr.length(); ++i) {
        if (!std::isdigit(portStr[i])) return false; // Non-digit character found
    }
    
    int port = std::atoi(portStr.c_str());
    return (port > 0 && port < 65536); // Valid port range
}

bool ConfigParser::_isValidErrorCode(const std::string &errorCodeStr) const {
    if (errorCodeStr.empty()) return false;
    
    for (size_t i = 0; i < errorCodeStr.length(); ++i) {
        if (!std::isdigit(errorCodeStr[i])) return false; // Non-digit character found
    }
    
    int errorCode = std::atoi(errorCodeStr.c_str());
    return (errorCode >= 300 && errorCode < 600); // Valid HTTP error code range
}

void ConfigParser::_parseAccessLogDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) {
    if (tokens.size() > 1) {
        currentServer.setAccessLog(tokens[1]);
    }
}

void ConfigParser::_parseErrorLogDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) {
    if (tokens.size() > 1) {
        currentServer.setErrorLog(tokens[1]);
    }
}

bool ConfigParser::_getNextDirective(std::ifstream& file, std::vector<std::string>& tokens) {
    std::string line;
    while (std::getline(file, line)) {
        line = _trim(line);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line == "}") {
            return false; // End of block
        }

        tokens = _split(line);
        if (tokens.empty()) {
            continue;
        }

        // Remove semicolon from last token if present
        if (!tokens.back().empty() && tokens.back()[tokens.back().length() - 1] == ';') {
            tokens.back() = tokens.back().substr(0, tokens.back().length() - 1);
        }
        return true; // Directive found
    }
    return false; // End of file
}

void ConfigParser::_parseServerBlock(std::ifstream& file) {
    ServerConfig currentServer;
    std::vector<std::string> tokens;

    while (_getNextDirective(file, tokens)) {
        std::string directive = tokens[0];
        
        if (directive == "listen") {
            _parseListenDirective(tokens, currentServer);
        }
        else if (directive == "root") {
            _parseRootDirective(tokens, currentServer);
        }
        else if (directive == "server_name") {
            _parseServerNameDirective(tokens, currentServer);
        }
        else if (directive == "error_page") {
            _parseErrorPageDirective(tokens, currentServer);
        }
        else if (directive == "index") {
            _parseIndexDirective(tokens, currentServer);
        }
        else if (directive == "client_max_body_size") {
            _parseClientMaxBodySizeDirective(tokens, currentServer);
        }
        else if (directive == "access_log") {
            _parseAccessLogDirective(tokens, currentServer);
        }
        else if (directive == "error_log") {
            _parseErrorLogDirective(tokens, currentServer);
        }
        else if (directive == "location") {
            std::string path = "";
            if (tokens.size() > 2) {
                for (size_t i = 1; i < tokens.size() - 1; ++i) {
                    path += tokens[i] + (i == tokens.size() - 2 ? "" : " ");
                }
            } else if (tokens.size() > 1) {
                path = tokens[1];
            }
            _parseLocationBlock(file, currentServer, path);
        }
    }
    if (currentServer.getPorts().empty()) {
        currentServer.addPort(8080);
    }
    _serverConfigs.push_back(currentServer);
}

void ConfigParser::_parseListenDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) const {
    if (tokens.size() < 2) {
        return; // Invalid listen directive, need at least "listen" and a value
    }

    for (size_t i = 1; i < tokens.size(); ++i) {
        std::string listenValue = tokens[i];
        
        // Handle "host:port" format
        if (listenValue.find(":") != std::string::npos) {
            size_t colonPos = listenValue.find(":");
            std::string host = listenValue.substr(0, colonPos);
            std::string port = listenValue.substr(colonPos + 1);
            
            if (!host.empty()) {
                currentServer.setHost(host);
            }
            if (_isValidPort(port)) {
                currentServer.addPort(std::atoi(port.c_str()));
            }
        }
        // Handle port-only format
        else if (_isValidPort(listenValue)) {
            currentServer.addPort(std::atoi(listenValue.c_str()));
        }
    }
}

void ConfigParser::_parseRootDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) const {
    if (tokens.size() < 2) {
        return; // Invalid root directive, need at least "root" and a path
    }
    
    currentServer.setRoot(tokens[1]);
}

void ConfigParser::_parseServerNameDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) const {
    if (tokens.size() < 2) {
        return; // Invalid server_name directive, need at least "server_name" and a name
    }
    
    // Add all server names from the directive (can be multiple)
    for (size_t i = 1; i < tokens.size(); ++i) {
        if (!tokens[i].empty()) {
            currentServer.addServerName(tokens[i]);
        }
    }
}

void ConfigParser::_parseErrorPageDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) const {
    if (tokens.size() < 3) {
        return; // Invalid error_page directive, need at least "error_page", error_code(s), and a page
    }
    
    // Last token is the error page path
    std::string errorPage = tokens[tokens.size() - 1];
    
    // Handle redirect syntax (error_page 404 = /custom_404.php)
    bool isRedirect = false;
    if (tokens.size() >= 4 && tokens[tokens.size() - 2] == "=") {
        isRedirect = true;
        errorPage = tokens[tokens.size() - 1];
    }
    
    // Parse error codes (all tokens between directive name and error page)
    std::vector<int> errorCodes;
    size_t endIndex = isRedirect ? tokens.size() - 2 : tokens.size() - 1;
    
    for (size_t i = 1; i < endIndex; ++i) {
        if (tokens[i] != "=" && _isValidErrorCode(tokens[i])) {
            errorCodes.push_back(std::atoi(tokens[i].c_str()));
        }
    }
    
    // Add error pages for all specified error codes
    if (!errorCodes.empty() && !errorPage.empty()) {
        currentServer.addErrorPages(errorCodes, errorPage);
    }
}

void ConfigParser::_parseIndexDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) {
    if (tokens.size() > 1) {
        std::vector<std::string> indexFiles;
        for (size_t i = 1; i < tokens.size(); ++i) {
            indexFiles.push_back(tokens[i]);
        }
        currentServer.setIndex(indexFiles);
    }
}

void ConfigParser::_parseClientMaxBodySizeDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) {
    if (tokens.size() > 1) {
        unsigned long size = 0;
        std::string sizeStr = tokens[1];
        char lastChar = sizeStr[sizeStr.length() - 1];
        sizeStr.erase(sizeStr.length() - 1);
        std::stringstream ss(sizeStr);
        ss >> size;
        if (lastChar == 'k' || lastChar == 'K') {
            size *= 1024;
        } else if (lastChar == 'm' || lastChar == 'M') {
            size *= 1024 * 1024;
        } else if (lastChar == 'g' || lastChar == 'G') {
            size *= 1024 * 1024 * 1024;
        }
        currentServer.setClientMaxBodySize(size);
    }
}

// location block
void ConfigParser::_parseLocationBlock(std::ifstream& file, ServerConfig& currentServer, const std::string& path) {
    LocationConfig newLocation;
    newLocation.setPath(path);
    std::vector<std::string> tokens;

    while (_getNextDirective(file, tokens)) {
        std::string directive = tokens[0];
        
        if (directive == "root") {
            _parseLocationRootDirective(tokens, newLocation);
        }
        else if (directive == "accepted_http_methods") {
            _parseAcceptedHttpMethodsDirective(tokens, newLocation);
        }
        else if (directive == "return") {
            _parseReturnDirective(tokens, newLocation);
        }
        else if (directive == "autoindex") {
            _parseAutoIndexDirective(tokens, newLocation);
        }
        else if (directive == "index") {
            _parseIndexDirective(tokens, newLocation);
        }
        else if (directive == "fastcgi_pass") {
            _parseFastCgiPassDirective(tokens, newLocation);
        }
        else if (directive == "fastcgi_param") {
            _parseFastCgiParamDirective(tokens, newLocation);
        }
        else if (directive == "fastcgi_index") {
            _parseFastCgiIndexDirective(tokens, newLocation);
        }
        else if (directive == "upload_path") {
            _parseUploadPathDirective(tokens, newLocation);
        }
        else if (directive == "try_files") {
            _parseTryFilesDirective(tokens, newLocation);
        }
    }
    currentServer.addLocation(newLocation);
}

void ConfigParser::_parseTryFilesDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation) {
    if (tokens.size() > 1) {
        for (size_t i = 1; i < tokens.size(); ++i) {
            newLocation.addTryFile(tokens[i]);
        }
    }
}

void ConfigParser::_parseLocationRootDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation) {
    if (tokens.size() > 1) {
        newLocation.setRoot(tokens[1]);
    }
}

void ConfigParser::_parseAcceptedHttpMethodsDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation) {
    if (tokens.size() > 1) {
        for (size_t i = 1; i < tokens.size(); ++i) {
            newLocation.addAllowedMethod(tokens[i]);
        }
    }
}

void ConfigParser::_parseReturnDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation) {
    if (tokens.size() > 1) {
        newLocation.setRedirect(tokens[1]);
    }
}

void ConfigParser::_parseAutoIndexDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation) {
    if (tokens.size() > 1 && tokens[1] == "on") {
        newLocation.setAutoIndex(true);
    }
}

void ConfigParser::_parseIndexDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation) {
    if (tokens.size() > 1) {
        newLocation.setIndex(tokens[1]);
    }
}

void ConfigParser::_parseFastCgiPassDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation) {
    if (tokens.size() > 1) {
        newLocation.setCgiPath(tokens[1]);
    }
}

void ConfigParser::_parseFastCgiParamDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation) {
    if (tokens.size() > 2) {
        newLocation.addCgiParam(tokens[1], tokens[2]);
    }
}

void ConfigParser::_parseFastCgiIndexDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation) {
    if (tokens.size() > 1) {
        newLocation.setCgiIndex(tokens[1]);
    }
}

void ConfigParser::_parseUploadPathDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation) {
    if (tokens.size() > 1) {
        newLocation.setUploadPath(tokens[1]);
    }
}
