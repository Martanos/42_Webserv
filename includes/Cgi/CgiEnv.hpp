#ifndef CGIENV_HPP
#define CGIENV_HPP

#include "../../includes/Config/Location.hpp"
#include "../../includes/Config/Server.hpp"
#include "../../includes/Http/HttpRequest.hpp"
#include <cstring>
#include <map>
#include <string>

// Initializes and manages the environment variables for the CGI process
class CgiEnv
{
private:
	std::map<std::string, std::string> _envVariables;
	std::string _convertHeaderNameToCgi(const std::string &headerName) const;

public:
	CgiEnv();
	CgiEnv(const CgiEnv &other);
	~CgiEnv();

	CgiEnv &operator=(const CgiEnv &other);
	void setEnv(const std::string &key, const std::string &value);
	std::string getEnv(const std::string &key) const;
	const std::map<std::string, std::string> &getEnvVariables() const;
	void printEnv() const;

	// Environment setup
	void _transposeData(const HttpRequest &request, const Server *server, const Location *location);

	// New methods for CGI execution

	// Environment array management for execve
	char **getEnvArray() const;
	void freeEnvArray(char **envArray) const;

	// Utility methods
	size_t getEnvCount() const;
	bool hasEnv(const std::string &key) const;
};

std::ostream &operator<<(std::ostream &os, const CgiEnv &env);

#endif
