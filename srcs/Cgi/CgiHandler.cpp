#include "../../includes/Cgi/CgiHandler.hpp"
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
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

// Main method to facilitate CGI execution
CgiHandler::ExecutionResult CgiHandler::execute(const HttpRequest &request, HttpResponse &response,
												const Server *server, const Location *location)
{
	// Reset internal redirect state
	_isInternalRedirect = false;
	_internalRedirectPath.clear();

	if (!server || !location)
	{
		Logger::error("Invalid server or location for CGI execution", __FILE__, __LINE__, __PRETTY_FUNCTION__);
		response.setResponseDefaultBody(500, "Invalid server or location for CGI execution", server, location,
										HttpResponse::ERROR);
		return ERROR_INTERNAL_ERROR;
	}

	// Resolve script path using RAW URI (HTTP target), not the filesystem path already translated
	std::string scriptPath = resolveCgiScriptPath(request.getRawUri(), server, location);
	Logger::debug("CgiHandler: Resolved script path: " + scriptPath, __FILE__, __LINE__, __PRETTY_FUNCTION__);
	// Skip pre-validation; executor will validate existence/executability.
	Logger::debug("CgiHandler: Skipping pre-validation; delegating to executor", __FILE__, __LINE__,
				  __PRETTY_FUNCTION__);

	// Setup CGI environment (uses raw URI semantics)
	Logger::debug("CgiHandler: Transposing environment", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	_cgiEnv._transposeData(request, server, location);
	Logger::debug("CgiHandler: Environment variable count after transpose: " +
					  StrUtils::toString(_cgiEnv.getEnvCount()),
				  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	// Override SCRIPT_FILENAME with resolved script path (SCRIPT_NAME remains logical path)
	_cgiEnv.setEnv("SCRIPT_FILENAME", scriptPath);
	Logger::debug("CgiHandler: SCRIPT_FILENAME set to: " + scriptPath, __FILE__, __LINE__, __PRETTY_FUNCTION__);

	// Determine interpreter
	Logger::debug("CgiHandler: Determining interpreter", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	std::string interpreter = determineInterpreter(scriptPath, location);
	Logger::debug("CgiHandler: Interpreter determined: " +
					  (interpreter.empty() ? std::string("(shebang/none)") : interpreter),
				  __FILE__, __LINE__, __PRETTY_FUNCTION__);

	// Execute CGI script
	std::string output, error;
	Logger::debug("CgiHandler: Executing CGI script", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	ExecutionResult result = executeCgiScript(scriptPath, interpreter, request, output, error);
	Logger::debug("CgiHandler: executeCgiScript returned result code: " + StrUtils::toString(result), __FILE__,
				  __LINE__, __PRETTY_FUNCTION__);
	if (result != SUCCESS)
	{
		// Set appropriate error response
		switch (result)
		{
		case ERROR_SCRIPT_NOT_FOUND:
			response.setResponseDefaultBody(404, "Script Not Found", server, location, HttpResponse::ERROR);
			break;
		case ERROR_TIMEOUT:
			response.setResponseDefaultBody(504, "Gateway Timeout", server, location, HttpResponse::ERROR);
			break;
		default:
			response.setResponseDefaultBody(500, "CGI Execution Failed", server, location, HttpResponse::ERROR);
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
			return SUCCESS; // Don't populate response, let caller handle redirect
		}
	}

	// Suppress body for 204/304 status codes
	int statusCode = _response.getStatusCode();
	if (statusCode == 204 || statusCode == 304)
	{
		// Clear body for these status codes
		_response.getHeaders(); // Keep headers but body will be cleared by populateHttpResponse
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
	return _timeout;
}

/*
** --------------------------------- PRIVATE ----------------------------------
*/

CgiHandler::ExecutionResult CgiHandler::executeCgiScript(const std::string &scriptPath, const std::string &interpreter,
														 const HttpRequest &request, std::string &output,
														 std::string &error)
{
	// Get environment array
	char **envArray = _cgiEnv.getEnvArray();
	if (!envArray)
	{
		Logger::log(Logger::ERROR, "Failed to create environment array for CGI");
		return ERROR_INTERNAL_ERROR;
	}

	// Execute the script
	CgiExecutor::ExecutionResult execResult =
		_executor.execute(scriptPath, interpreter, envArray, request.getBodyData(), output, error);

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
		return SUCCESS;
	case CgiExecutor::ERROR_SCRIPT_NOT_FOUND:
	case CgiExecutor::ERROR_SCRIPT_NOT_EXECUTABLE:
		return ERROR_SCRIPT_NOT_FOUND;
	case CgiExecutor::ERROR_TIMEOUT:
		return ERROR_TIMEOUT;
	default:
		return ERROR_EXECUTION_FAILED;
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

		return ERROR_RESPONSE_PARSING_FAILED;
	}

	// Populate HTTP response from parsed CGI response
	_response.populateHttpResponse(response);

	return SUCCESS;
}

std::string CgiHandler::determineInterpreter(const std::string &scriptPath, const Location *location) const
{
	(void)scriptPath; // Interpreter detection via shebang by default

	// If location cgi_path points to an executable FILE (legacy style), use it as interpreter.
	// If it points to a DIRECTORY (preferred), do not set interpreter here; executor will use shebang.
	if (location)
	{
		std::string configured = location->getCgiPath();
		if (!configured.empty())
		{
			struct stat st;
			if (stat(configured.c_str(), &st) == 0 && S_ISREG(st.st_mode))
			{
				return configured; // treat as interpreter binary
			}
		}
	}

	// Let executor infer interpreter via shebang
	return "";
}

bool CgiHandler::validateScriptPath(const std::string &scriptPath) const
{
	if (scriptPath.empty())
	{
		return false;
	}

	// Check if file exists and is accessible
	struct stat st;
	if (stat(scriptPath.c_str(), &st) != 0)
	{
		return false;
	}

	// Check if it's a regular file
	if (!S_ISREG(st.st_mode))
	{
		return false;
	}

	// Check if it's readable
	if (!(st.st_mode & S_IRUSR))
	{
		return false;
	}
	return true;
}

void CgiHandler::logExecutionDetails(const HttpRequest &request, const std::string &scriptPath,
									 ExecutionResult result) const
{
	std::string logMessage =
		"CGI execution: " + request.getMethod() + " " + request.getUri() + " -> " + scriptPath + " (";

	switch (result)
	{
	case SUCCESS:
		logMessage += "SUCCESS";
		Logger::log(Logger::INFO, logMessage + ")");
		break;
	case ERROR_INVALID_SCRIPT_PATH:
		logMessage += "INVALID_SCRIPT_PATH";
		Logger::log(Logger::ERROR, logMessage + ")");
		break;
	case ERROR_SCRIPT_NOT_FOUND:
		logMessage += "SCRIPT_NOT_FOUND";
		Logger::log(Logger::ERROR, logMessage + ")");
		break;
	case ERROR_EXECUTION_FAILED:
		logMessage += "EXECUTION_FAILED";
		Logger::log(Logger::ERROR, logMessage + ")");
		break;
	case ERROR_RESPONSE_PARSING_FAILED:
		logMessage += "RESPONSE_PARSING_FAILED";
		Logger::log(Logger::ERROR, logMessage + ")");
		break;
	case ERROR_TIMEOUT:
		logMessage += "TIMEOUT";
		Logger::log(Logger::ERROR, logMessage + ")");
		break;
	case ERROR_INTERNAL_ERROR:
		logMessage += "INTERNAL_ERROR";
		Logger::log(Logger::ERROR, logMessage + ")");
		break;
	default:
		logMessage += "UNKNOWN_ERROR";
		Logger::log(Logger::ERROR, logMessage + ")");
		break;
	}
}

bool CgiHandler::isInternalRedirect() const
{
	return _isInternalRedirect;
}

std::string CgiHandler::getInternalRedirectPath() const
{
	return _internalRedirectPath;
}

bool CgiHandler::isInternalRedirectPath(const std::string &location) const
{
	// Internal redirect if:
	// - Starts with '/' (absolute path)
	// - AND no scheme (no "http://", "https://", etc.)
	// - AND status is 200 (not explicitly a 3xx redirect)

	if (location.empty() || location[0] != '/')
	{
		return false; // External or relative
	}

	if (location.find("://") != std::string::npos)
	{
		return false; // Has scheme, external
	}

	// If script explicitly sets 3xx status, treat as external
	int statusCode = _response.getStatusCode();
	if (statusCode >= 300 && statusCode < 400)
	{
		return false; // Explicit redirect status
	}

	return true; // Internal redirect
}

std::string CgiHandler::resolveCgiScriptPath(const std::string &uri, const Server *server, const Location *location)
{
	// Resolve filesystem path for CGI script using cgi_path directory when provided.
	// Take the part of the URI after the matched location path and append to cgi_path.
	std::string cleanUri = StrUtils::sanitizeUriPath(uri);

	if (location && !location->getCgiPath().empty())
	{
		std::string base = location->getCgiPath(); // already normalized (no trailing slash) by translator
		std::string locPrefix = location->getLocationPath();
		// Ensure locPrefix ends with '/'
		if (!locPrefix.empty() && locPrefix[locPrefix.size() - 1] != '/')
			locPrefix += "/";

		std::string tail = cleanUri;
		// Strip the location prefix from URI if present
		if (cleanUri.find(locPrefix) == 0)
			tail = cleanUri.substr(locPrefix.size());

		// Prevent directory traversal in tail (sanitizeUriPath returns absolute path)
		// Remove any leading slashes from tail for safe concatenation
		while (!tail.empty() && tail[0] == '/')
			tail.erase(0, 1);

		return base + "/" + tail;
	}

	// Fallback: resolve relative to location root or server root
	std::string scriptPath;
	if (location && !location->getRootPath().empty())
		scriptPath = location->getRootPath() + cleanUri;
	else if (server && !server->getRootPath().empty())
		scriptPath = server->getRootPath() + cleanUri;
	else
		scriptPath = cleanUri;

	return scriptPath;
}

/* ************************************************************************** */
