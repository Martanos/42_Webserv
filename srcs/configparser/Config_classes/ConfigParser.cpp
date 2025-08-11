#include "ConfigParser.hpp"

ConfigParser::ConfigParser() {}


ConfigParser::ConfigParser(const std::string& filename) {
    parseConfig(filename);
}

ConfigParser::~ConfigParser() {}

ConfigParser& ConfigParser::operator=(const ConfigParser &other) {
    if (this != &other) {
        _serverConfigs = other._serverConfigs;
    }
    return *this;
}

ConfigParser::ConfigParser(const ConfigParser &other) {
    *this = other;
}

bool ConfigParser::parseConfig(const std::string &filename) {
    std::ifstream file(filename.c_str());
    std::stringstream errorMessage;
    if (!file.is_open()) {
        errorMessage << "Error opening file: " << filename;
        Logger::log(Logger::ERROR, errorMessage.str());
        throw std::runtime_error(errorMessage.str());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();  // Load entire file into stringstream for efficient parsing
    file.close();

    std::string line;
    bool foundHttp = false;
    bool insideHttp = false;
    bool insideServer = false;
    double lineNumber = 0; // Initialize line number for error reporting
    while(std::getline(buffer, line)) {
        lineNumber++;
        if (line.empty() || line[0] == '#') {
            continue; // Skip empty lines and comments
        }
        line = _trim(line);
        
        httpblockcheck(line, foundHttp, insideHttp);
        
        if (serverblockcheck(line, insideHttp, insideServer)) {
            _parseServerBlock(buffer, lineNumber); // Handle server block start
        }
        
        // Handle closing brace for http block (when not inside server)
        if (insideHttp && !insideServer && line == "}") {
            insideHttp = false;
        }
    }
    if (_serverConfigs.empty()) {
        errorMessage << "No valid server blocks found, terminating parser";
        Logger::log(Logger::ERROR, errorMessage.str());
        throw std::runtime_error(errorMessage.str());
    }
    return true;
}

bool ConfigParser::serverblockcheck(const std::string &line, bool &insideHttp, bool &insideServer) {
    std::stringstream errorMessage;

    if (line == "server {" || line == "server{") {
        if (!insideHttp) {
            errorMessage << "Error: 'server' directive is not allowed here. Server blocks must be inside an http block.";
            Logger::log(Logger::ERROR, errorMessage.str());
            throw std::runtime_error(errorMessage.str());
        }
        insideServer = true;
        return true; // Start of server block
    }

    if (insideServer && line == "}") {
        insideServer = false;
        return false; // End of server block - don't trigger parseServerBlock
    }
    return false; // Not a server block directive
}

bool ConfigParser::httpblockcheck(const std::string &line, bool &foundHttp, bool &insideHttp) {
    std::stringstream errorMessage;

    if (line == "http {" || line == "http{") {
        if (foundHttp) {
            errorMessage << "Error: Multiple http blocks found. Only one http block is allowed.";
            Logger::log(Logger::ERROR, errorMessage.str());
            throw std::runtime_error(errorMessage.str());
        }
        foundHttp = true;
        insideHttp = true;
        return true; // Start of http block
    }

    // Note: http block closing is now handled by checking if we're not inside a server block
    // This prevents the ambiguity of which block a "}" is closing
    return false; // Not a http block directive
}


void ConfigParser::_parseServerBlock(std::stringstream &buffer, double &lineNumber) {
    ServerConfig currentServer;
    std::string line;
    
    while (std::getline(buffer, line)) {
        line = _trim(line);
        lineNumber++;

        if (line.empty() || line[0] == '#') {
            continue; // Skip empty lines and comments
        }
        
        if (line == "}") {
            break; // End of server block
        }
        
        // Basic server directive parsing - can be expanded later
        std::vector<std::string> tokens = _split(line);
        if (tokens.empty()) {
            continue; // Skip empty lines
        }
        std::string directive = tokens[0];
        
        switch (_getServerDirectiveType(directive)) {
            case SERVER_LISTEN:
                currentServer.addHosts_ports(line, lineNumber);
                break;
            case SERVER_NAME:
                currentServer.addServerName(line, lineNumber);
                break;
            case SERVER_ROOT:
                currentServer.addRoot(line, lineNumber);
                break;
            case SERVER_INDEX:
                currentServer.addIndexes(line, lineNumber);
                break;
            case SERVER_ERROR_PAGE:
                currentServer.addErrorPages(line, lineNumber);
                break;
            case SERVER_CLIENT_MAX_BODY_SIZE:
                currentServer.addClientMaxBodySize(line, lineNumber);
                break;
            case SERVER_AUTOINDEX:
                currentServer.addAutoindex(line, lineNumber);
                break;
            case SERVER_ACCESS_LOG:
                currentServer.addAccessLog(line, lineNumber);
                break;
            case SERVER_ERROR_LOG:
                currentServer.addErrorLog(line, lineNumber);
                break;
            case SERVER_LOCATION:
                // TODO: Implement location parsing
                break;
            case SERVER_UNKNOWN:
            default:
                // TODO: Handle unknown directive or log warning
                break;
        }
        // For now, just create a basic server config

    }
    
    // Add the parsed server to our collection
    _serverConfigs.push_back(currentServer);
}

std::string ConfigParser::_trim(const std::string &str) const {
    // First, remove inline comments
    std::string cleaned = str;
    size_t commentPos = cleaned.find('#');
    if (commentPos != std::string::npos) {
        cleaned = cleaned.substr(0, commentPos);
    }
    
    size_t first = cleaned.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return ""; // No non-whitespace characters
    size_t last = cleaned.find_last_not_of(" \t\n\r");
    return cleaned.substr(first, (last - first + 1));
}

std::vector<std::string> ConfigParser::_split(const std::string &str) const {
    std::vector<std::string> tokens;
    std::istringstream stream(str);
    std::string token;
    
    while (stream >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

void ConfigParser::printAllConfigs() const
{
    std::cout << "=== Configuration Summary ===" << std::endl;
    std::cout << "Total servers configured: " << _serverConfigs.size() << std::endl;
    std::cout << std::endl;

    for (size_t i = 0; i < _serverConfigs.size(); ++i)
    {
        std::cout << "--- Server " << (i + 1) << " ---" << std::endl;
        _serverConfigs[i].printConfig();
        std::cout << std::endl;
    }
}

ConfigParser::ServerDirectiveType ConfigParser::_getServerDirectiveType(const std::string &directive) const {
    if (directive == "listen") return SERVER_LISTEN;
    if (directive == "server_name") return SERVER_NAME;
    if (directive == "root") return SERVER_ROOT;
    if (directive == "index") return SERVER_INDEX;
    if (directive == "error_page") return SERVER_ERROR_PAGE;
    if (directive == "client_max_body_size") return SERVER_CLIENT_MAX_BODY_SIZE;
    if (directive == "autoindex") return SERVER_AUTOINDEX;
    if (directive == "access_log") return SERVER_ACCESS_LOG;
    if (directive == "error_log") return SERVER_ERROR_LOG;
    if (directive == "location") return SERVER_LOCATION;
    return SERVER_UNKNOWN;
}
