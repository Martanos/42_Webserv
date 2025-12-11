#ifndef IMETHODHANDLER_HPP
#define IMETHODHANDLER_HPP

#include "../../includes/Cgi/CgiHandler.hpp"
#include "../../includes/Config/Location.hpp"
#include "../../includes/Http/HttpRequest.hpp"
#include "../../includes/Http/HttpResponse.hpp"
#include "../../includes/Utils/FileUtils.hpp"

// Abstract base class for HTTP method handlers
class IMethodHandler
{
public:
	virtual ~IMethodHandler()
	{
	}

	// Pure virtual method to handle the request
	virtual bool handleRequest(const HttpRequest &request, HttpResponse &response, const Location &location) = 0;

protected:
	// Shared CGI execution helper for all method handlers
	static bool executeCgi(const std::string &scriptPath, const HttpRequest &request, HttpResponse &response,
						   const Location &location)
	{
		if (!FileUtils::isFileExecutable(scriptPath))
		{
			response.setResponseDefaultBody(403, "Forbidden: CGI script is not executable", &location,
											HttpResponse::ERROR);
			return false;
		}
		CgiHandler cgi;
		CgiHandler::ExecutionResult result =
			cgi.execute(scriptPath, request, response, &location, request.getSelectedServer());
		return result == CgiHandler::SUCCESS;
	}
};

#endif /* IMETHODHANDLER_HPP */
