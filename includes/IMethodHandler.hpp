#ifndef IMETHODHANDLER_HPP
#define IMETHODHANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Server.hpp"
#include "Location.hpp"

class IMethodHandler
{
public:
	virtual ~IMethodHandler() {}

	// Pure virtual method to handle the request
	virtual void handle(const HttpRequest &request,
						HttpResponse &response,
						const Server *server,
						const Location *location = NULL) = 0;

	// Check if this handler can process the request
	virtual bool canHandle(const std::string &method) const = 0;

	// Get supported method name
	virtual std::string getMethod() const = 0;
};

#endif
