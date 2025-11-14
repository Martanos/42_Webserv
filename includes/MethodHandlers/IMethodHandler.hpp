#ifndef IMETHODHANDLER_HPP
#define IMETHODHANDLER_HPP

#include "../../includes/Config/Location.hpp"
#include "../../includes/Config/Server.hpp"
#include "../../includes/Http/HttpRequest.hpp"
#include "../../includes/Http/HttpResponse.hpp"

// Abstract base class for HTTP method handlers
class IMethodHandler
{
public:
	virtual ~IMethodHandler()
	{
	}

	// Pure virtual method to handle the request
	virtual bool handleRequest(const HttpRequest &request, HttpResponse &response, const Server *server,
							   const Location *location) = 0;

	// Virtual method to check if this handler can handle the given method
	virtual bool canHandle(const std::string &method) const = 0;
};

#endif /* IMETHODHANDLER_HPP */
