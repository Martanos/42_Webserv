#ifndef CGIENV_HPP
#define CGIENV_HPP

#include "../Core/Location.hpp"
#include "../Core/Server.hpp"
#include "../HTTP/HttpRequest.hpp"
#include <cstring>
#include <map>
#include <string>

// Initializes and manages the environment variables for the CGI process
class CGIenv
{
private:
	std::map<std::string, std::string> _envVariables;

	void _transposeData(const HttpRequest &request, const Server *server, const Location *location);

public:
	CGIenv();
	CGIenv(const CGIenv &other);
	~CGIenv();

	CGIenv &operator=(const CGIenv &other);

	void setEnv(const std::string &key, const std::string &value);
	std::string getEnv(const std::string &key) const;
	void printEnv() const;

	// New methods for CGI execution
	void setupFromRequest(const HttpRequest &request, const Server *server, const Location *location,
						  const std::string &scriptPath);
	void setupHttpHeaders(const HttpRequest &request);
	std::string convertHeaderNameToCgi(const std::string &headerName) const;

	// Environment array management for execve
	char **getEnvArray() const;
	void freeEnvArray(char **envArray) const;

	// Utility methods
	size_t getEnvCount() const;
	bool hasEnv(const std::string &key) const;
};

#endif
