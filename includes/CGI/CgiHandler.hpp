#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "CgiEnv.hpp"
#include "CgiExecutor.hpp"
#include "CgiResponse.hpp"
#include "../HTTP/HttpRequest.hpp"
#include "../HTTP/HttpResponse.hpp"
#include "../Core/Location.hpp"
#include "../Global/Logger.hpp"
#include "../Core/Server.hpp"
#include <string>

class CgiHandler
{
public:
	enum ExecutionResult
	{
		SUCCESS = 0,
		ERROR_INVALID_SCRIPT_PATH = 1,
		ERROR_SCRIPT_NOT_FOUND = 2,
		ERROR_EXECUTION_FAILED = 3,
		ERROR_RESPONSE_PARSING_FAILED = 4,
		ERROR_TIMEOUT = 5,
		ERROR_INTERNAL_ERROR = 6
	};

private:
	static const int DEFAULT_TIMEOUT = 30; // seconds

	int _timeout;
	CGIenv _cgiEnv;
	CgiExecutor _executor;
	CgiResponse _response;

public:
	CgiHandler();
	CgiHandler(int timeout);
	CgiHandler(const CgiHandler &other);
	~CgiHandler();

	CgiHandler &operator=(const CgiHandler &other);

	// Main execution method
	ExecutionResult execute(const HttpRequest &request, HttpResponse &response, const Server *server,
							const Location *location);

	// Configuration
	void setTimeout(int seconds);
	int getTimeout() const;

	// Utility methods
	static std::string resolveCgiScriptPath(const std::string &uri, const Server *server, const Location *location);

private:
	// Internal methods
	ExecutionResult setupEnvironment(const HttpRequest &request, const Server *server, const Location *location,
									 const std::string &scriptPath);

	ExecutionResult executeCgiScript(const std::string &scriptPath, const std::string &interpreter,
									 const HttpRequest &request, std::string &output, std::string &error);

	ExecutionResult processResponse(const std::string &output, const std::string &error, HttpResponse &response,
									const Server *server);

	// Utility methods
	std::string determineInterpreter(const std::string &scriptPath, const Location *location) const;
	bool validateScriptPath(const std::string &scriptPath) const;
	void logExecutionDetails(const HttpRequest &request, const std::string &scriptPath, ExecutionResult result) const;
};

#endif /* CGIHANDLER_HPP */
