#include "../../includes/CGI/CgiHandler.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include <algorithm>
#include <sys/stat.h>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

CgiHandler::CgiHandler() : _timeout(DEFAULT_TIMEOUT), _executor(_timeout)
{
}

CgiHandler::CgiHandler(int timeout) : _timeout(timeout), _executor(timeout)
{
}

CgiHandler::CgiHandler(const CgiHandler &other)
	: _timeout(other._timeout), _cgiEnv(other._cgiEnv), _executor(other._timeout), _response(other._response)
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
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

CgiHandler::ExecutionResult CgiHandler::execute(const HttpRequest &request, HttpResponse &response,
												const Server *server, const Location *location)
{
	if (!server || !location)
	{
		Logger::log(Logger::ERROR, "Invalid server or location for CGI execution");
		return ERROR_INTERNAL_ERROR;
	}

	// Resolve the CGI script path
	std::string scriptPath = resolveCgiScriptPath(request.getUri(), server, location);
	if (scriptPath.empty())
	{
		Logger::log(Logger::ERROR, "Could not resolve CGI script path for URI: " + request.getUri());
		return ERROR_INVALID_SCRIPT_PATH;
	}

	// Validate script path
	if (!validateScriptPath(scriptPath))
	{
		Logger::log(Logger::ERROR, "Invalid or inaccessible CGI script: " + scriptPath);
		return ERROR_SCRIPT_NOT_FOUND;
	}

	// Setup CGI environment
	ExecutionResult result = setupEnvironment(request, server, location, scriptPath);
	if (result != SUCCESS)
	{
		return result;
	}

	// Determine interpreter
	std::string interpreter = determineInterpreter(scriptPath, location);

	// Execute CGI script
	std::string output, error;
	result = executeCgiScript(scriptPath, interpreter, request, output, error);
	if (result != SUCCESS)
	{
		logExecutionDetails(request, scriptPath, result);
		return result;
	}

	// Process the response
	result = processResponse(output, error, response, server);

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

std::string CgiHandler::resolveCgiScriptPath(const std::string &uri, const Server *server, const Location *location)
{
	// Get the location path and root
	std::string locationPath = location->getPath();
	std::string root = location->getRoot();
	if (root.empty())
		root = server->getRootPath();

	// Remove location path from URI to get the script relative path
	std::string scriptRelativePath;
	if (uri.length() > locationPath.length() && uri.substr(0, locationPath.length()) == locationPath)
	{
		scriptRelativePath = uri.substr(locationPath.length());

		// Remove leading slash if present
		if (!scriptRelativePath.empty() && scriptRelativePath[0] == '/')
		{
			scriptRelativePath = scriptRelativePath.substr(1);
		}
	}

	// If no script path specified, look for default script
	if (scriptRelativePath.empty())
	{
		scriptRelativePath = "index.cgi"; // Default CGI script name
	}

	// Construct full script path
	std::string fullPath = root;
	if (!fullPath.empty() && fullPath[fullPath.length() - 1] != '/')
	{
		fullPath += '/';
	}
	fullPath += scriptRelativePath;

	return fullPath;
}

/*
** --------------------------------- PRIVATE ----------------------------------
*/

CgiHandler::ExecutionResult CgiHandler::setupEnvironment(const HttpRequest &request, const Server *server,
														 const Location *location, const std::string &scriptPath)
{
	_cgiEnv.copyDataFromServer(server, location);
	_cgiEnv.setupFromRequest(request, server, location, scriptPath);
	return SUCCESS;
}

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
	(void)scriptPath; // TODO: Use scriptPath for interpreter detection
	// First, check if location specifies an interpreter
	if (location)
	{
		std::string cgiPath = location->getCgiPath();
		if (!cgiPath.empty())
		{
			return cgiPath;
		}
	}

	// If no interpreter specified, let CgiExecutor handle shebang detection
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

/* ************************************************************************** */
