#include "LocationConfig.hpp"

LocationConfig::LocationConfig() : _autoIndex(false) {}

LocationConfig::~LocationConfig() {}

LocationConfig::LocationConfig(const LocationConfig &other) {
    _path = other._path;
    _root = other._root;
    _allowedMethods = other._allowedMethods;
    _redirect = other._redirect;
    _autoIndex = other._autoIndex;
    _index = other._index;
    _cgiPath = other._cgiPath;
    _cgiIndex = other._cgiIndex;
    _cgiParams = other._cgiParams;
    _uploadPath = other._uploadPath;
    _tryFiles = other._tryFiles;
}

LocationConfig& LocationConfig::operator=(const LocationConfig &other) {
    if (this != &other) {
        _path = other._path;
        _root = other._root;
        _allowedMethods = other._allowedMethods;
        _redirect = other._redirect;
        _autoIndex = other._autoIndex;
        _index = other._index;
        _cgiPath = other._cgiPath;
        _cgiIndex = other._cgiIndex;
        _cgiParams = other._cgiParams;
        _uploadPath = other._uploadPath; 
        _tryFiles = other._tryFiles;
    }
    return *this;
}

// Getters
const std::string& LocationConfig::getPath() const { return _path; }
const std::string& LocationConfig::getRoot() const { return _root; }
const std::vector<std::string>& LocationConfig::getAllowedMethods() const { return _allowedMethods; }
const std::string& LocationConfig::getRedirect() const { return _redirect; }
bool LocationConfig::isAutoIndexEnabled() const { return _autoIndex; }
const std::string& LocationConfig::getIndex() const { return _index; }
const std::string& LocationConfig::getCgiPath() const { return _cgiPath; }
const std::string& LocationConfig::getCgiIndex() const { return _cgiIndex; }
const std::map<std::string, std::string>& LocationConfig::getCgiParams() const { return _cgiParams; }
const std::string& LocationConfig::getUploadPath() const { return _uploadPath; }
const std::vector<std::string>& LocationConfig::getTryFiles() const { return _tryFiles; }

// Setters
void LocationConfig::setPath(const std::string& path) { _path = path; }
void LocationConfig::setRoot(const std::string& root) { _root = root; }
void LocationConfig::addAllowedMethod(const std::string& method) { _allowedMethods.push_back(method); }
void LocationConfig::setRedirect(const std::string& redirect) { _redirect = redirect; }
void LocationConfig::setAutoIndex(bool autoIndex) { _autoIndex = autoIndex; }
void LocationConfig::setIndex(const std::string& index) { _index = index;}
void LocationConfig::setCgiPath(const std::string& cgiPath) { _cgiPath = cgiPath; }
void LocationConfig::setCgiIndex(const std::string& cgiIndex) { _cgiIndex = cgiIndex; }
void LocationConfig::addCgiParam(const std::string& param, const std::string& value) { _cgiParams[param] = value; }
void LocationConfig::setUploadPath(const std::string& uploadPath) { _uploadPath = uploadPath; }
void LocationConfig::addTryFile(const std::string& file) { _tryFiles.push_back(file); }

void LocationConfig::printConfig() const {
    std::cout << "Location Path: " << _path << std::endl;
    if (!_root.empty()) std::cout << "    Root: " << _root << std::endl;
    if (!_allowedMethods.empty()) {
        std::cout << "    Allowed Methods: ";
        for (size_t i = 0; i < _allowedMethods.size(); ++i) {
            std::cout << _allowedMethods[i] << (i == _allowedMethods.size() - 1 ? "" : ", ");
        }
        std::cout << std::endl;
    }
    if (!_redirect.empty()) std::cout << "    Redirect: " << _redirect << std::endl;
    std::cout << "    Auto Index: " << (_autoIndex ? "on" : "off") << std::endl;
    if (!_index.empty()) std::cout << "    Index: " << _index << std::endl;
    if (!_cgiPath.empty()) std::cout << "    CGI Path: " << _cgiPath << std::endl;
    if (!_cgiIndex.empty()) std::cout << "    CGI Index: " << _cgiIndex << std::endl;
    if (!_cgiParams.empty()) {
        std::cout << "    CGI Params:" << std::endl;
        for (std::map<std::string, std::string>::const_iterator it = _cgiParams.begin(); it != _cgiParams.end(); ++it) {
            std::cout << "        " << it->first << ": " << it->second << std::endl;
        }
    }
    if (!_uploadPath.empty()) std::cout << "    Upload Path: " << _uploadPath << std::endl;
    if (!_tryFiles.empty()) {
        std::cout << "    Try Files: ";
        for (size_t i = 0; i < _tryFiles.size(); ++i) {
            std::cout << _tryFiles[i] << (i == _tryFiles.size() - 1 ? "" : ", ");
        }
        std::cout << std::endl;
    }
}
