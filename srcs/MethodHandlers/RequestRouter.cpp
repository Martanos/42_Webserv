#include "RequestRouter.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

RequestRouter::RequestRouter()
{
}

RequestRouter::RequestRouter(const RequestRouter &src)
{
	(void)src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

RequestRouter::~RequestRouter()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

RequestRouter &RequestRouter::operator=(RequestRouter const &rhs)
{
	(void)rhs;
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void RequestRouter::route(const HttpRequest &request,
						  HttpResponse &response,
						  const Server *server)
{
	if (!server)
	{
		Logger::log(Logger::ERROR, "No server configuration provided to router");
		generateErrorResponse(response, 500, NULL);
		return;
	}

	// Get the request method and URI
	std::string method = request.getMethod();
	std::string uri = request.getUri();

	// Log the request
	Logger::log(Logger::INFO, "Routing " + method + " request for " + uri);

	// Match location configuration
	const Location *location = matchLocation(uri, server);

	// Check if method is allowed at this location
	if (!isMethodAllowed(method, location, server))
	{
		response.setStatus(405, "Method Not Allowed");
		response.setBody(server->getStatusPage(405));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length",
						   StringUtils::toString(response.getBody().length()));
		response.setHeader("Allow", getAllowedMethodsString(location, server));
		return;
	}

	// Get appropriate handler from factory
	MethodHandlerFactory &factory = MethodHandlerFactory::getInstance();
	IMethodHandler *handler = factory.getHandler(method);

	if (!handler)
	{
		// Method not implemented
		response.setStatus(501, "Not Implemented");
		response.setBody(server->getStatusPage(501));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length",
						   StringUtils::toString(response.getBody().length()));
		return;
	}

	try
	{
		// Delegate to the appropriate handler
		handler->handle(request, response, server, location);

		// Add server header
		response.setHeader("Server", "42_Webserv/1.0");

		// Add Date header
		time_t now = time(NULL);
		char dateBuffer[100];
		struct tm *tm = gmtime(&now);
		strftime(dateBuffer, sizeof(dateBuffer),
				 "%a, %d %b %Y %H:%M:%S GMT", tm);
		response.setHeader("Date", std::string(dateBuffer));

		// Add Connection header based on keep-alive settings
		std::string connection = request.getHeader("connection")[0];
		if (connection == "close" || request.getVersion() == "HTTP/1.0")
		{
			response.setHeader("Connection", "close");
		}
		else
		{
			response.setHeader("Connection", "keep-alive");
		}
	}
	catch (const std::exception &e)
	{
		Logger::log(Logger::ERROR, "Exception during request handling: " +
									   std::string(e.what()));
		generateErrorResponse(response, 500, server);
	}
}

const Location *RequestRouter::matchLocation(const std::string &uri,
											 const Server *server) const
{
	if (!server)
	{
		return NULL;
	}

	const std::map<std::string, Location> &locations = server->getLocations();

	// Find the best matching location (longest prefix match)
	const Location *bestMatch = NULL;
	size_t bestMatchLength = 0;

	for (std::map<std::string, Location>::const_iterator it = locations.begin();
		 it != locations.end(); ++it)
	{
		const std::string &path = it->first;

		// Check if URI starts with this location path
		if (uri.find(path) == 0)
		{
			// Check if it's a better match than what we have
			if (path.length() > bestMatchLength)
			{
				bestMatch = &(it->second);
				bestMatchLength = path.length();
			}
		}
	}

	return bestMatch;
}

bool RequestRouter::isMethodAllowed(const std::string &method,
									const Location *location,
									const Server *server) const
{
	(void)server;

	// If no location is matched, allow basic methods
	if (!location)
	{
		return method == "GET" || method == "HEAD";
	}

	// Check location's allowed methods
	const std::vector<std::string> &allowedMethods = location->getAllowedMethods();

	// If no methods specified, allow GET and HEAD by default
	if (allowedMethods.empty())
	{
		return method == "GET" || method == "HEAD";
	}

	// Check if method is in allowed list
	for (std::vector<std::string>::const_iterator it = allowedMethods.begin();
		 it != allowedMethods.end(); ++it)
	{
		if (*it == method)
		{
			return true;
		}
		// HEAD is allowed if GET is allowed
		if (*it == "GET" && method == "HEAD")
		{
			return true;
		}
	}

	return false;
}

std::string RequestRouter::getAllowedMethodsString(const Location *location,
												   const Server *server) const
{
	(void)server;

	std::string allowed;

	if (!location || location->getAllowedMethods().empty())
	{
		return "GET, HEAD";
	}

	const std::vector<std::string> &methods = location->getAllowedMethods();
	for (size_t i = 0; i < methods.size(); ++i)
	{
		if (i > 0)
		{
			allowed += ", ";
		}
		allowed += methods[i];
		// Add HEAD if GET is allowed
		if (methods[i] == "GET")
		{
			allowed += ", HEAD";
		}
	}

	return allowed;
}

void RequestRouter::generateErrorResponse(HttpResponse &response,
										  int statusCode,
										  const Server *server) const
{
	response.setStatus(statusCode, DefaultStatusMap::getStatusMessage(statusCode));

	// Try to get custom error page from server
	std::string errorBody;
	if (server)
	{
		errorBody = server->getStatusPage(statusCode);
	}

	// Fall back to default if no custom page
	if (errorBody.empty())
	{
		errorBody = DefaultStatusMap::getStatusBody(statusCode);
	}

	response.setBody(errorBody);
	response.setHeader("Content-Type", "text/html");
	response.setHeader("Content-Length", StringUtils::toString(errorBody.length()));
	response.setHeader("Server", "42_Webserv/1.0");
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

/* ************************************************************************** */
