#include "ConfigParser.hpp"

ConfigParser::ServerDirectiveType ConfigParser::_getServerDirectiveType(const std::string& directive) const {
    if (directive == "listen") return SERVER_LISTEN;
    if (directive == "root") return SERVER_ROOT;
    if (directive == "server_name") return SERVER_NAME;
    if (directive == "error_page") return SERVER_ERROR_PAGE;
    if (directive == "index") return SERVER_INDEX;
    if (directive == "client_max_body_size") return SERVER_CLIENT_MAX_BODY_SIZE;
    if (directive == "access_log") return SERVER_ACCESS_LOG;
    if (directive == "error_log") return SERVER_ERROR_LOG;
    return SERVER_UNKNOWN;
}

ConfigParser::LocationDirectiveType ConfigParser::_getLocationDirectiveType(const std::string& directive) const {
    if (directive == "root") return LOCATION_ROOT;
    if (directive == "allowed_methods") return LOCATION_ACCEPTED_HTTP_METHODS;
    if (directive == "return") return LOCATION_RETURN;
    if (directive == "autoindex") return LOCATION_AUTOINDEX;
    if (directive == "index") return LOCATION_INDEX;
    if (directive == "fastcgi_pass") return LOCATION_FASTCGI_PASS;
    if (directive == "fastcgi_param") return LOCATION_FASTCGI_PARAM;
    if (directive == "fastcgi_index") return LOCATION_FASTCGI_INDEX;
    if (directive == "upload_path") return LOCATION_UPLOAD_PATH;
    if (directive == "try_files") return LOCATION_TRY_FILES;
    return LOCATION_UNKNOWN;
}

bool ConfigParser::_isValidPort(const std::string &portStr) const {
    if (portStr.empty()) return false;
    
    size_t start = 0;
    if (portStr[0] == '-' || portStr[0] == '+') {
        start = 1; // Skip the sign
        if (portStr.length() == 1) return false; // Just a sign is not valid
    }
    
    for (size_t i = start; i < portStr.length(); ++i) {
        if (!std::isdigit(portStr[i])) return false; // Non-digit character found
    }
    
    return true; // Valid numeric value
}

bool ConfigParser::_isWhitespaceOnly(const std::string &str) const {
    if (str.empty()) return true;
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (!std::isspace(str[i])) return false; // Non-whitespace character found
    }
    
    return true; // String contains only whitespace
}

bool ConfigParser::_isValidErrorCode(const std::string &errorCodeStr) const {
    if (errorCodeStr.empty()) return false;
    
    for (size_t i = 0; i < errorCodeStr.length(); ++i) {
        if (!std::isdigit(errorCodeStr[i])) return false; // Non-digit character found
    }
    
    int errorCode = std::atoi(errorCodeStr.c_str());
    return (errorCode);
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

// Overloaded version for stringstream - more efficient
bool ConfigParser::_getNextDirective(std::stringstream& stream, std::vector<std::string>& tokens) {
    std::string line;
    while (std::getline(stream, line)) {
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
        
        // Handle location blocks separately since they need special processing
        if (directive == "location") {
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
        // Use switch for server directives
        else {
            switch (_getServerDirectiveType(directive)) {
                case SERVER_LISTEN:
                    _parseListenDirective(tokens, currentServer);
                    break;
                case SERVER_ROOT:
                    _parseRootDirective(tokens, currentServer);
                    break;
                case SERVER_NAME:
                    _parseServerNameDirective(tokens, currentServer);
                    break;
                case SERVER_ERROR_PAGE:
                    _parseErrorPageDirective(tokens, currentServer);
                    break;
                case SERVER_INDEX:
                    _parseIndexDirective(tokens, currentServer);
                    break;
                case SERVER_CLIENT_MAX_BODY_SIZE:
                    _parseClientMaxBodySizeDirective(tokens, currentServer);
                    break;
                case SERVER_ACCESS_LOG:
                    _parseAccessLogDirective(tokens, currentServer);
                    break;
                case SERVER_ERROR_LOG:
                    _parseErrorLogDirective(tokens, currentServer);
                    break;
                case SERVER_UNKNOWN:
                default:
                    // Log unknown server directives
                    std::cerr << "Warning: Unknown server directive '" << directive << "' ignored" << std::endl;
                    break;
            }
        }
    }
    
    // Set default port if none specified
    if (currentServer.getPorts().empty()) {
        currentServer.addPort(8080);
    }
    _serverConfigs.push_back(currentServer);
}

// Efficient stringstream version
void ConfigParser::_parseServerBlock(std::stringstream& stream) {
    ServerConfig currentServer;
    std::vector<std::string> tokens;

    while (_getNextDirective(stream, tokens)) {
        std::string directive = tokens[0];
        
        // Handle location blocks separately since they need special processing
        if (directive == "location") {
            std::string path = "";
            if (tokens.size() > 2) {
                for (size_t i = 1; i < tokens.size() - 1; ++i) {
                    path += tokens[i] + (i == tokens.size() - 2 ? "" : " ");
                }
            } else if (tokens.size() > 1) {
                path = tokens[1];
            }
            _parseLocationBlock(stream, currentServer, path);
        }
        // Use switch for server directives
        else {
            switch (_getServerDirectiveType(directive)) {
                case SERVER_LISTEN:
                    _parseListenDirective(tokens, currentServer);
                    break;
                case SERVER_ROOT:
                    _parseRootDirective(tokens, currentServer);
                    break;
                case SERVER_NAME:
                    _parseServerNameDirective(tokens, currentServer);
                    break;
                case SERVER_ERROR_PAGE:
                    _parseErrorPageDirective(tokens, currentServer);
                    break;
                case SERVER_INDEX:
                    _parseIndexDirective(tokens, currentServer);
                    break;
                case SERVER_CLIENT_MAX_BODY_SIZE:
                    _parseClientMaxBodySizeDirective(tokens, currentServer);
                    break;
                case SERVER_ACCESS_LOG:
                    _parseAccessLogDirective(tokens, currentServer);
                    break;
                case SERVER_ERROR_LOG:
                    _parseErrorLogDirective(tokens, currentServer);
                    break;
                case SERVER_UNKNOWN:
                default:
                    // Log unknown server directives
                    std::cerr << "Warning: Unknown server directive '" << directive << "' ignored" << std::endl;
                    break;
            }
        }
    }
    
    // Set default port if none specified
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
        std::cerr << "Error: Invalid root directive, missing path value" << std::endl;
        return; // Invalid root directive, need at least "root" and a path
    }
    
    std::string rootPath = tokens[1];
    
    // Remove surrounding quotes if present
    if (rootPath.length() >= 2 && rootPath[0] == '"' && rootPath[rootPath.length() - 1] == '"') {
        rootPath = rootPath.substr(1, rootPath.length() - 2);
    }
    
    if (_isWhitespaceOnly(rootPath)) {
        std::cerr << "Error: Invalid root directive, path cannot be empty or whitespace only" << std::endl;
        return;
    }
    
    currentServer.setRoot(rootPath);
}

void ConfigParser::_parseServerNameDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) const {
    if (tokens.size() < 2) {
        std::cerr << "Error: Invalid server_name directive, missing server name value" << std::endl;
        return; // Invalid server_name directive, need at least "server_name" and a name
    }
    
    // Add all server names from the directive (can be multiple)
    bool hasValidName = false;
    for (size_t i = 1; i < tokens.size(); ++i) {
        std::string serverName = tokens[i];
        
        // Remove surrounding quotes if present
        if (serverName.length() >= 2 && serverName[0] == '"' && serverName[serverName.length() - 1] == '"') {
            serverName = serverName.substr(1, serverName.length() - 2);
        }
        
        if (!_isWhitespaceOnly(serverName)) { // This catches both empty strings and whitespace-only strings
            currentServer.addServerName(serverName);
            hasValidName = true;
        }
    }
    
    if (!hasValidName) {
        std::cerr << "Error: Invalid server_name directive, all server names are empty or whitespace only" << std::endl;
    }
}

void ConfigParser::_parseErrorPageDirective(const std::vector<std::string>& tokens, ServerConfig& currentServer) const {
    if (tokens.size() < 3) {
        std::cerr << "Error: Invalid error_page directive, need at least error code and page path" << std::endl;
        return; // Invalid error_page directive, need at least "error_page", error_code(s), and a page
    }
    
    // Last token is the error page path
    std::string errorPage = tokens[tokens.size() - 1];
    
    // Remove surrounding quotes if present
    if (errorPage.length() >= 2 && errorPage[0] == '"' && errorPage[errorPage.length() - 1] == '"') {
        errorPage = errorPage.substr(1, errorPage.length() - 2);
    }
    
    if (_isWhitespaceOnly(errorPage)) {
        std::cerr << "Error: Invalid error_page directive, error page path cannot be empty or whitespace only" << std::endl;
        return;
    }
    
    // Handle redirect syntax (error_page 404 = /custom_404.php)
    bool isRedirect = false;
    if (tokens.size() >= 4 && tokens[tokens.size() - 2] == "=") {
        isRedirect = true;
        errorPage = tokens[tokens.size() - 1];
        
        // Remove surrounding quotes if present
        if (errorPage.length() >= 2 && errorPage[0] == '"' && errorPage[errorPage.length() - 1] == '"') {
            errorPage = errorPage.substr(1, errorPage.length() - 2);
        }
        
        if (_isWhitespaceOnly(errorPage)) {
            std::cerr << "Error: Invalid error_page directive, error page path cannot be empty or whitespace only" << std::endl;
            return;
        }
    }
    
    // Parse error codes (all tokens between directive name and error page)
    std::vector<int> errorCodes;
    size_t endIndex = isRedirect ? tokens.size() - 2 : tokens.size() - 1;
    
    bool hasValidErrorCode = false;
    for (size_t i = 1; i < endIndex; ++i) {
        if (tokens[i] != "=") {
            if (_isValidErrorCode(tokens[i])) {
                errorCodes.push_back(std::atoi(tokens[i].c_str()));
                hasValidErrorCode = true;
            } else {
                std::cerr << "Error: Invalid error code '" << tokens[i] << "' in error_page directive" << std::endl;
                return;
            }
        }
    }
    
    if (!hasValidErrorCode) {
        std::cerr << "Error: Invalid error_page directive, no valid error codes specified" << std::endl;
        return;
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
        
        switch (_getLocationDirectiveType(directive)) {
            case LOCATION_ROOT:
                _parseLocationRootDirective(tokens, newLocation);
                break;
            case LOCATION_ACCEPTED_HTTP_METHODS:
                _parseAcceptedHttpMethodsDirective(tokens, newLocation);
                break;
            case LOCATION_RETURN:
                _parseReturnDirective(tokens, newLocation);
                break;
            case LOCATION_AUTOINDEX:
                _parseAutoIndexDirective(tokens, newLocation);
                break;
            case LOCATION_INDEX:
                _parseIndexDirective(tokens, newLocation);
                break;
            case LOCATION_FASTCGI_PASS:
                _parseFastCgiPassDirective(tokens, newLocation);
                break;
            case LOCATION_FASTCGI_PARAM:
                _parseFastCgiParamDirective(tokens, newLocation);
                break;
            case LOCATION_FASTCGI_INDEX:
                _parseFastCgiIndexDirective(tokens, newLocation);
                break;
            case LOCATION_UPLOAD_PATH:
                _parseUploadPathDirective(tokens, newLocation);
                break;
            case LOCATION_TRY_FILES:
                _parseTryFilesDirective(tokens, newLocation);
                break;
            case LOCATION_UNKNOWN:
            default:
                // Log unknown location directives
                std::cerr << "Warning: Unknown location directive '" << directive << "' ignored" << std::endl;
                break;
        }
    }
    currentServer.addLocation(newLocation);
}

// Efficient stringstream version
void ConfigParser::_parseLocationBlock(std::stringstream& stream, ServerConfig& currentServer, const std::string& path) {
    LocationConfig newLocation;
    newLocation.setPath(path);
    std::vector<std::string> tokens;

    while (_getNextDirective(stream, tokens)) {
        std::string directive = tokens[0];
        
        switch (_getLocationDirectiveType(directive)) {
            case LOCATION_ROOT:
                _parseLocationRootDirective(tokens, newLocation);
                break;
            case LOCATION_ACCEPTED_HTTP_METHODS:
                _parseAcceptedHttpMethodsDirective(tokens, newLocation);
                break;
            case LOCATION_RETURN:
                _parseReturnDirective(tokens, newLocation);
                break;
            case LOCATION_AUTOINDEX:
                _parseAutoIndexDirective(tokens, newLocation);
                break;
            case LOCATION_INDEX:
                _parseIndexDirective(tokens, newLocation);
                break;
            case LOCATION_FASTCGI_PASS:
                _parseFastCgiPassDirective(tokens, newLocation);
                break;
            case LOCATION_FASTCGI_PARAM:
                _parseFastCgiParamDirective(tokens, newLocation);
                break;
            case LOCATION_FASTCGI_INDEX:
                _parseFastCgiIndexDirective(tokens, newLocation);
                break;
            case LOCATION_UPLOAD_PATH:
                _parseUploadPathDirective(tokens, newLocation);
                break;
            case LOCATION_TRY_FILES:
                _parseTryFilesDirective(tokens, newLocation);
                break;
            case LOCATION_UNKNOWN:
            default:
                // Log unknown location directives
                std::cerr << "Warning: Unknown location directive '" << directive << "' ignored" << std::endl;
                break;
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
    if (tokens.size() < 2) {
        std::cerr << "Error: Invalid allowed_methods directive, missing HTTP method value" << std::endl;
        return; // Invalid allowed_methods directive, need at least "allowed_methods" and a method
    }
    
    bool hasValidMethod = false;
    for (size_t i = 1; i < tokens.size(); ++i) {
        std::string method = tokens[i];
        
        // Skip tokens that are just quotes or empty/whitespace
        if (method == "\"" || _isWhitespaceOnly(method)) {
            continue; // Skip quote tokens and empty/whitespace-only tokens
        }
        
        // Check if the method is valid (e.g., GET, POST, PUT, DELETE)
        if (method != "GET" && method != "POST" && method != "PUT" &&
            method != "DELETE" && method != "HEAD" && method != "OPTIONS") {
            std::cerr << "Warning: Invalid HTTP method '" << method << "' in allowed_methods directive, ignored." << std::endl;
            continue; // Skip invalid methods
        }
        
        newLocation.addAllowedMethod(method);
        hasValidMethod = true;
    }
    
    if (!hasValidMethod) {
        std::cerr << "Error: Invalid allowed_methods directive, all HTTP methods are empty or whitespace only" << std::endl;
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

// truncating whitespaces like tab
void ConfigParser::_parseUploadPathDirective(const std::vector<std::string>& tokens, LocationConfig& newLocation) {
    if (tokens.size() > 1) {
        if (tokens[1].empty() || _isWhitespaceOnly(tokens[1])) {
            std::cerr << "Error: Invalid upload_path directive, path cannot be empty or whitespace only" << std::endl;
            // newLocation.setUploadPath(""); // Reset to empty if invalid
        }

        newLocation.setUploadPath(tokens[1]);
       
    }
}
