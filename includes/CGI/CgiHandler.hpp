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
	CgiEnv _cgiEnv;
	CgiExecutor _executor;
	CgiResponse _response;

	// Internal redirect state
	bool _isInternalRedirect;
	std::string _internalRedirectPath;

private:
	// Internal methods
	ExecutionResult executeCgiScript(const std::string &scriptPath, const std::string &interpreter,
									 const HttpRequest &request, std::string &output, std::string &error);

	ExecutionResult processResponse(const std::string &output, const std::string &error, HttpResponse &response,
									const Server *server);

	// Utility methods
	std::string determineInterpreter(const std::string &scriptPath, const Location *location) const;
	bool validateScriptPath(const std::string &scriptPath) const;
	void logExecutionDetails(const HttpRequest &request, const std::string &scriptPath, ExecutionResult result) const;
	bool isInternalRedirectPath(const std::string &location) const;

public:
	CgiHandler();
	CgiHandler(int timeout);
	CgiHandler(const CgiHandler &other);
	~CgiHandler();

	CgiHandler &operator=(const CgiHandler &other);

	// Main execution method - returns ExecutionResult and modifies HttpResponse directly
	ExecutionResult execute(const HttpRequest &request, HttpResponse &response, const Server *server,
							const Location *location);

	// Configuration
	void setTimeout(int seconds);
	int getTimeout() const;

	// Internal redirect detection
	bool isInternalRedirect() const;
	std::string getInternalRedirectPath() const;

	// Utility methods
	static std::string resolveCgiScriptPath(const std::string &uri, const Server *server, const Location *location);
};

#endif /* CGIHANDLER_HPP */
