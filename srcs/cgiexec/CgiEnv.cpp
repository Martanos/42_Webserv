#include "CGIenv.hpp"

CGIenv::CGIenv() {}

CGIenv::~CGIenv() {}



void CGIenv::setEnv(const std::string &key, const std::string &value) {
    if (key.empty() || value.empty()) {
        return; // Do not set empty keys or values
    }
    // Set environment variable
    _envVariables[key] = value; // Store in map for internal use
}

std::string CGIenv::getEnv(const std::string &key) const {
    char *value = _envVariables.find(key); // Get value from map
    return value ? std::string(value) : std::string();
}

void CGIenv::copyDataFromServer( const ServerConfig &server, const LocationConfig &location) {
    setEnv("SERVER_NAME", server.getHost());
    setEnv("SERVER_PORT", server.getPorts().front()); // Assuming first port is the primary one
    setEnv("SCRIPT_NAME", location.getPath());
    setEnv("PATH_TRANSLATED", ""); // This would typically be the full path to the script, but we leave it empty for now    
    
    
    // from http request
    setEnv("REQUEST_METHOD", "GET"); // Default, can be changed later
    setEnv("QUERY_STRING", "");
    setEnv("CONTENT_TYPE", ""); // Set later if needed
    setEnv("CONTENT_LENGTH", "0"); // Default, can be changed later

    setEnv("REMOTE_ADDR", ""); // change later when we have client info
    setEnv("SERVER_PROTOCOL", "HTTP/1.1");

}

void CGIenv::printEnv() const {
    std::map<std::string, std::string>::const_iterator it;
    for (it = _envVariables.begin(); it != _envVariables.end(); ++it) {
        std::cout << it->first << ": " << it->second << std::endl;
    }
}
