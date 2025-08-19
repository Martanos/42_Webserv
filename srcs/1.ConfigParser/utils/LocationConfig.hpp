#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>

class LocationConfig {
    private:
        std::string _path;
        std::string _root;
        std::vector<std::string> _allowedMethods;
        std::string _redirect;
        bool _autoIndex;
        std::string _index;
        std::string _cgiPath;
        std::string _cgiIndex;
        std::map<std::string, std::string> _cgiParams;
        std::string _uploadPath;
        std::vector<std::string> _tryFiles;

    public:
        LocationConfig(const LocationConfig &other);
        LocationConfig& operator=(const LocationConfig &other);

        LocationConfig();
        ~LocationConfig();

        const std::string& getPath() const;
        const std::string& getRoot() const;
        const std::vector<std::string>& getAllowedMethods() const;
        const std::string& getRedirect() const;
        bool isAutoIndexEnabled() const;
        const std::string& getIndex() const;
        const std::string& getCgiPath() const;
        const std::string& getCgiIndex() const;
        const std::map<std::string, std::string>& getCgiParams() const;
        const std::string& getUploadPath() const;
        const std::vector<std::string>& getTryFiles() const;

        void setPath(const std::string& path);
        void setRoot(const std::string& root);
        void addAllowedMethod(const std::string& method);
        void setRedirect(const std::string& redirect);
        void setAutoIndex(bool autoIndex);
        void setIndex(const std::string& index);
        void setCgiPath(const std::string& cgiPath);
        void setCgiIndex(const std::string& cgiIndex);
        void addCgiParam(const std::string& param, const std::string& value);
        void setUploadPath(const std::string& uploadPath);
        void addTryFile(const std::string& file);
        
        void printConfig() const;
};

#endif