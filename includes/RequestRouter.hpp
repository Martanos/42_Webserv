#ifndef REQUESTROUTER_HPP
#define REQUESTROUTER_HPP

#include "DefaultStatusMap.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Location.hpp"
#include "Logger.hpp"
#include "MethodHandlerFactory.hpp"
#include "RequestRouter.hpp"
#include "Server.hpp"
#include "StringUtils.hpp"
#include <algorithm>

class RequestRouter
{
private:
	// Match the best location for a given URI
	const Location *matchLocation(const std::string &uri, const Server *server) const;

	// Check if method is allowed at location
	bool isMethodAllowed(const std::string &method, const Location *location, const Server *server) const;

	// Generate error response with proper error page
	void generateErrorResponse(HttpResponse &response, int statusCode, const Server *server) const;

	// Get allowed methods string for Allow header
	std::string getAllowedMethodsString(const Location *location, const Server *server) const;

public:
	RequestRouter();
	~RequestRouter();

	// Main routing method
	void route(const HttpRequest &request, HttpResponse &response, const Server *server);
};

#endif
