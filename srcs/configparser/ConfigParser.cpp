#include "ConfigParser.hpp"

ConfigParser::ConfigParser() {}

ConfigParser::ConfigParser(const std::string& filename) {
    parseConfig(filename);
}

ConfigParser::~ConfigParser() {}

bool ConfigParser::parseConfig(const std::string& filename) {
    
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = _trim(line);

        if (line.empty() || line[0] == '#') {
            continue; // Skip empty lines and comments
        }

        if (line == "server {" || line == "server{") {
            _parseServerBlock(file);
        }
    }
    
    file.close();
    
    // If no servers were parsed, create default
    if (_serverConfigs.empty()) {
        std::cerr << "No valid server blocks found, terminating parser" << std::endl;
    }
    
    return true;
}

const std::vector<ServerConfig>& ConfigParser::getServerConfigs() const {
    return _serverConfigs;
}

void ConfigParser::printAllConfigs() const {
    std::cout << "Number of server configurations: " << _serverConfigs.size() << std::endl;
    std::vector<ServerConfig>::const_iterator it = _serverConfigs.begin();
    int i = 0;
    for (; it != _serverConfigs.end(); ++it, ++i) {
        std::cout << "\n Server " << (i + 1) << std::endl;
        it->printConfig();
    }
}

std::string ConfigParser::_trim(const std::string &str) const {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return ""; // No non-whitespace characters
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
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