#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include "Logger.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class LocationConfig
{
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

	// Private static utilities
	static void throwConfigError(const std::string& msg, const char* file, int line);
	static bool validateDirective(std::stringstream& stream, const std::string& expectedDirective, double lineNumber, const char* file, int line);
	static void lineValidation(std::string &line, int lineNumber);

public:
	LocationConfig();
	~LocationConfig();
	LocationConfig(const LocationConfig &other);
	LocationConfig &operator=(const LocationConfig &other);
	bool operator==(const LocationConfig &other) const;

	// Getters
	const std::string &getPath() const;
	const std::string &getRoot() const;
	const std::vector<std::string> &getAllowedMethods() const;
	const std::string &getRedirect() const;
	bool isAutoIndexEnabled() const;
	const std::string &getIndex() const;
	const std::string &getCgiPath() const;
	const std::string &getCgiIndex() const;
	const std::map<std::string, std::string> &getCgiParams() const;
	const std::string &getUploadPath() const;
	const std::vector<std::string> &getTryFiles() const;

	// Parsing methods (standalone pattern - takes whole line and line number)
	void addPath(std::string line, double lineNumber);
	void addRoot(std::string line, double lineNumber);
	void addAllowedMethods(std::string line, double lineNumber);
	void addRedirect(std::string line, double lineNumber);
	void addAutoIndex(std::string line, double lineNumber);
	void addIndex(std::string line, double lineNumber);
	void addCgiPath(std::string line, double lineNumber);
	void addCgiIndex(std::string line, double lineNumber);
	void addCgiParam(std::string line, double lineNumber);
	void addUploadPath(std::string line, double lineNumber);

	// Debugging methods
	void printConfig() const;
};

#endif
