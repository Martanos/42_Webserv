#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "../../includes/Config/Location.hpp"
#include "../../includes/Config/Server.hpp"
#include "../../includes/Http/HttpRequest.hpp"
#include "../../includes/Http/HttpResponse.hpp"
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
	// Configuration
	int _timeout;
	CgiEnv _cgiEnv;
	CgiExecutor _executor;
	CgiResponse _response;
	ExecutionResult _lastResult;

	// Internal redirect state
	bool _isInternalRedirect;
	std::string _internalRedirectPath;

private:
	// Internal methods
	ExecutionResult executeCgiScript(const std::string &scriptPath, const HttpRequest &request, std::string &output,
									 std::string &error);

	ExecutionResult processResponse(const std::string &output, const std::string &error, HttpResponse &response,
									const Server *server);

	// Utility methods
	void logExecutionDetails(const HttpRequest &request, const std::string &scriptPath, ExecutionResult result) const;
	bool isInternalRedirectPath(const std::string &location) const;
	bool validateScriptPath(const std::string &scriptPath) const;

public:
	static const int DEFAULT_TIMEOUT = 30;

	CgiHandler();
	explicit CgiHandler(int timeout);
	CgiHandler(const CgiHandler &other);
	~CgiHandler();

	CgiHandler &operator=(const CgiHandler &other);

	// Main execution method - returns ExecutionResult and modifies HttpResponse directly
	ExecutionResult execute(const std::string &scriptPath, const HttpRequest &request, HttpResponse &response,
							const Location *location, const Server *server = NULL);

	// Static utility for path resolution
	static std::string resolveCgiScriptPath(const std::string &uri, const Server *server, const Location *location);

	// Configuration
	void setTimeout(int seconds);
	int getTimeout() const;

	// Internal redirect detection
	bool isInternalRedirect() const;
	std::string getInternalRedirectPath() const;
};

#endif /* CGIHANDLER_HPP */
