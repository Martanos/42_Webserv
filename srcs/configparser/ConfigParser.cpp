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

    // Load entire file into stringstream for efficient parsing
    std::stringstream buffer;
    buffer << file.rdbuf();  // Single system call - much faster!
    file.close();

    std::string line;
    while (std::getline(buffer, line)) {
        line = _trim(line);

        if (line.empty() || line[0] == '#') {
            continue; // Skip empty lines and comments
        }

        if (line == "server {" || line == "server{") {
            _parseServerBlock(buffer);  // Use efficient stringstream version
        }
    }
    
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