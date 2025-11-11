#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "../Core/Location.hpp"
#include "../Core/Server.hpp"
#include "../HTTP/HttpRequest.hpp"
#include "../HTTP/HttpResponse.hpp"
#include "CgiEnv.hpp"
#include "CgiExecutor.hpp"
#include "CgiResponse.hpp"
#include <string>

class CgiHandler
{
private:
	static const int DEFAULT_TIMEOUT = 30; // seconds

	int _timeout;
	CgiEnv _cgiEnv;
	CgiExecutor _executor;

private:
	// Internal methods
	void executeCgiScript(const std::string &scriptPath, const std::string &interpreter, const HttpRequest &request,
						  std::string &output, std::string &error);

	void processResponse(const std::string &output, const std::string &error, HttpResponse &response,
						 const Server *server);

	// Utility methods
	std::string determineInterpreter(const std::string &scriptPath, const Location *location) const;
	bool validateScriptPath(const std::string &scriptPath) const;

public:
	CgiHandler();
	CgiHandler(int timeout);
	CgiHandler(const CgiHandler &other);
	~CgiHandler();

	CgiHandler &operator=(const CgiHandler &other);

	// Main execution method
	void execute(const HttpRequest &request, HttpResponse &response, const Server *server, const Location *location);

	// Configuration
	void setTimeout(int seconds);
	int getTimeout() const;

	// Utility methods
	static std::string resolveCgiScriptPath(const std::string &uri, const Server *server, const Location *location);


};

#endif /* CGIHANDLER_HPP */
