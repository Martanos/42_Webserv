#include "../../includes/Cgi/CgiHandler.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Utils/StrUtils.hpp"
#include <sys/stat.h>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

CgiHandler::CgiHandler() : _timeout(DEFAULT_TIMEOUT), _executor(_timeout), _isInternalRedirect(false)
{
}

CgiHandler::CgiHandler(int timeout) : _timeout(timeout), _executor(timeout), _isInternalRedirect(false)
{
}

CgiHandler::CgiHandler(const CgiHandler &other)
	: _timeout(other._timeout), _cgiEnv(other._cgiEnv), _executor(other._timeout), _response(other._response),
	  _isInternalRedirect(other._isInternalRedirect), _internalRedirectPath(other._internalRedirectPath)
{
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

CgiHandler::~CgiHandler()
{
}

/*
** --------------------------------- OPERATORS --------------------------------
*/

CgiHandler &CgiHandler::operator=(const CgiHandler &other)
{
	if (this != &other)
	{
		_timeout = other._timeout;
		_cgiEnv = other._cgiEnv;
		_executor = other._executor;
		_response = other._response;
		_isInternalRedirect = other._isInternalRedirect;
		_internalRedirectPath = other._internalRedirectPath;
	}
	return (*this);
}

/*
** --------------------------------- METHODS ----------------------------------
*/

// Main method to facilitate CGI execution
CgiHandler::ExecutionResult CgiHandler::execute(const std::string &scriptPath, const HttpRequest &request,
												HttpResponse &response, const Location *location, const Server *server)
{
	ExecutionResult result;

	// Reset internal redirect state
	_isInternalRedirect = false;
	_internalRedirectPath.clear();

	// Validate script path
	if (scriptPath.empty())
	{
		Logger::error("CgiHandler: Empty script path", __FILE__, __LINE__, __PRETTY_FUNCTION__);
		response.setResponseDefaultBody(500, "Internal Server Error: Invalid CGI path", location, HttpResponse::ERROR);
		return ERROR_INVALID_SCRIPT_PATH;
	}

	// Setup CGI environment
	Logger::debug("CgiHandler: Transposing environment for script: " + scriptPath, __FILE__, __LINE__,
				  __PRETTY_FUNCTION__);
	_cgiEnv._transposeData(scriptPath, request, location);
	Logger::debug("CgiHandler: Environment variable count after transpose: " +
					  StrUtils::toString(_cgiEnv.getEnvCount()),
				  __FILE__, __LINE__, __PRETTY_FUNCTION__);

	// Execute CGI script
	std::string output, error;
	Logger::debug("CgiHandler: Executing CGI script: " + scriptPath, __FILE__, __LINE__, __PRETTY_FUNCTION__);
	result = executeCgiScript(scriptPath, request, output, error);
	Logger::debug("CgiHandler: executeCgiScript returned result code: " + StrUtils::toString(result), __FILE__,
				  __LINE__, __PRETTY_FUNCTION__);

	if (result != SUCCESS)
	{
		// Set appropriate error response
		switch (result)
		{
		case ERROR_SCRIPT_NOT_FOUND:
			response.setResponseDefaultBody(404, "Script Not Found", location, HttpResponse::ERROR);
			break;
		case ERROR_TIMEOUT:
			response.setResponseDefaultBody(504, "Gateway Timeout", location, HttpResponse::ERROR);
			break;
		default:
			response.setResponseDefaultBody(500, "CGI Execution Failed", location, HttpResponse::ERROR);
			break;
		}
		logExecutionDetails(request, scriptPath, result);
		return result;
	}

	// Process the response
	Logger::debug("CgiHandler: Processing CGI response", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	result = processResponse(output, error, response, server);
	Logger::debug("CgiHandler: processResponse returned result code: " + StrUtils::toString(result), __FILE__, __LINE__,
				  __PRETTY_FUNCTION__);

	// Check for internal redirect BEFORE finalizing response
	if (result == SUCCESS && _response.hasHeader("location"))
	{
		std::string locationValue = _response.getHeader("location");
		if (isInternalRedirectPath(locationValue))
		{
			_isInternalRedirect = true;
			_internalRedirectPath = locationValue;
			Logger::info("CGI internal redirect detected: " + locationValue, __FILE__, __LINE__, __PRETTY_FUNCTION__);
			logExecutionDetails(request, scriptPath, result);
			// Don't populate the response buffer; let the caller decide how to handle the redirect.
			return SUCCESS;
		}
	}

	// Suppress body for 204/304 status codes
	int statusCode = _response.getStatusCode();
	if (statusCode == 204 || statusCode == 304)
	{
		// Body will be cleared by populateHttpResponse for these status codes
		Logger::debug("CgiHandler: Suppressing body for status " + StrUtils::toString(statusCode), __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);
	}

	logExecutionDetails(request, scriptPath, result);
	return result;
}

void CgiHandler::setTimeout(int seconds)
{
	_timeout = seconds;
	_executor.setTimeout(seconds);
}

int CgiHandler::getTimeout() const
{
	return (_timeout);
}

/*
** --------------------------------- PRIVATE ----------------------------------
*/

CgiHandler::ExecutionResult CgiHandler::executeCgiScript(const std::string &scriptPath, const HttpRequest &request,
														 std::string &output, std::string &error)
{
	char **envArray;

	// Get environment array
	envArray = _cgiEnv.getEnvArray();
	if (!envArray)
	{
		Logger::log(Logger::ERROR, "Failed to create environment array for CGI");
		return (ERROR_INTERNAL_ERROR);
	}
	// Execute the script
	CgiExecutor::ExecutionResult execResult =
		_executor.execute(scriptPath, std::string(), envArray, request.getBody().getRawBody(), output, error);
	// Clean up environment array
	_cgiEnv.freeEnvArray(envArray);
	// Log stderr if present
	if (!error.empty())
	{
		Logger::warning("CGI script stderr output: " + error, __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	// Map executor results to handler results
	switch (execResult)
	{
	case CgiExecutor::SUCCESS:
		return (SUCCESS);
	case CgiExecutor::ERROR_SCRIPT_NOT_FOUND:
	case CgiExecutor::ERROR_SCRIPT_NOT_EXECUTABLE:
		return (ERROR_SCRIPT_NOT_FOUND);
	case CgiExecutor::ERROR_TIMEOUT:
		return (ERROR_TIMEOUT);
	default:
		return (ERROR_EXECUTION_FAILED);
	}
}

CgiHandler::ExecutionResult CgiHandler::processResponse(const std::string &output, const std::string &error,
														HttpResponse &response, const Server *server)
{
	(void)server;
	// Log any error output from CGI script
	if (!error.empty())
	{
		Logger::log(Logger::WARNING, "CGI script error output: " + error);
	}
	// Parse CGI output
	CgiResponse::ParseResult parseResult = _response.parseOutput(output);
	if (parseResult != CgiResponse::SUCCESS)
	{
		Logger::log(Logger::ERROR, "Failed to parse CGI output");
		// Return 500 error
		response.setStatus(500, "Internal Server Error");
		response.setBody("Internal Server Error");
		response.setHeader(Header("Content-Type: text/html"));
		response.setHeader(Header("Content-Length: " + StrUtils::toString(response.getBody().length())));
		return (ERROR_RESPONSE_PARSING_FAILED);
	}
	// Populate HTTP response from parsed CGI response
	_response.populateHttpResponse(response);
	return (SUCCESS);
}

bool CgiHandler::validateScriptPath(const std::string &scriptPath) const
{
	if (scriptPath.empty())
	{
		return (false);
	}

	// Check if file exists and is accessible
	struct stat st;
	if (stat(scriptPath.c_str(), &st) != 0)
	{
		return (false);
	}

	// Check if it's a regular file
	if (!S_ISREG(st.st_mode))
	{
		return (false);
	}

	// Check if it's readable
	if (!(st.st_mode & S_IRUSR))
	{
		return (false);
	}
	return (true);
}

void CgiHandler::logExecutionDetails(const HttpRequest &request, const std::string &scriptPath,
									 ExecutionResult result) const
{
	const HttpURI &uri = request.getURI();
	std::string logMessage =
		"CGI execution: " + uri.getMethod() + " " + uri.getResolvedPath() + " -> " + scriptPath + " (";

	// Map result to string
	const char *resultStr;
	switch (result)
	{
	case SUCCESS:
		resultStr = "SUCCESS";
		break;
	case ERROR_INVALID_SCRIPT_PATH:
		resultStr = "INVALID_SCRIPT_PATH";
		break;
	case ERROR_SCRIPT_NOT_FOUND:
		resultStr = "SCRIPT_NOT_FOUND";
		break;
	case ERROR_EXECUTION_FAILED:
		resultStr = "EXECUTION_FAILED";
		break;
	case ERROR_RESPONSE_PARSING_FAILED:
		resultStr = "RESPONSE_PARSING_FAILED";
		break;
	case ERROR_TIMEOUT:
		resultStr = "TIMEOUT";
		break;
	case ERROR_INTERNAL_ERROR:
		resultStr = "INTERNAL_ERROR";
		break;
	default:
		resultStr = "UNKNOWN_ERROR";
		break;
	}

	logMessage += resultStr;
	logMessage += ")";
	Logger::log(result == SUCCESS ? Logger::INFO : Logger::ERROR, logMessage);
}

bool CgiHandler::isInternalRedirect() const
{
	return (_isInternalRedirect);
}

std::string CgiHandler::getInternalRedirectPath() const
{
	return (_internalRedirectPath);
}

bool CgiHandler::isInternalRedirectPath(const std::string &location) const
{
	// Internal redirect if:
	// - Starts with '/' (absolute path)
	// - AND no scheme (no "http://", "https://", etc.)
	// - AND status is 200 (not explicitly a 3xx redirect)

	if (location.empty() || location[0] != '/')
	{
		return (false); // External or relative
	}

	if (location.find("://") != std::string::npos)
	{
		return (false); // Has scheme, external
	}

	// If script explicitly sets 3xx status, treat as external
	int statusCode = _response.getStatusCode();
	if (statusCode >= 300 && statusCode < 400)
	{
		return (false); // Explicit redirect status
	}

	return (true); // Internal redirect
}

std::string CgiHandler::resolveCgiScriptPath(const std::string &uri, const Server *server, const Location *location)
{
	std::string cleanUri = StrUtils::sanitizeUriPath(uri);
	const std::string *basePath = NULL;

	if (location && location->getRootPath() && !location->getRootPath()->empty())
		basePath = location->getRootPath();
	else if (server && server->getRootPath() && !server->getRootPath()->empty())
		basePath = server->getRootPath();

	if (!basePath || basePath->empty())
		return cleanUri;

	std::string normalizedBase = *basePath;
	if (!normalizedBase.empty() && normalizedBase[normalizedBase.size() - 1] == '/')
		normalizedBase.erase(normalizedBase.size() - 1);

	if (cleanUri.compare(0, normalizedBase.size(), normalizedBase) == 0)
		return cleanUri;

	if (!cleanUri.empty() && cleanUri[0] != '/')
		return normalizedBase + "/" + cleanUri;

	return normalizedBase + cleanUri;
}

/* ************************************************************************** */
