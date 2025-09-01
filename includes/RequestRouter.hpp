#ifndef REQUESTROUTER_HPP
#define REQUESTROUTER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Server.hpp"
#include "MethodHandlerFactory.hpp"

class RequestRouter
{
private:
	const Location *_matchLocation(const std::string &uri,
								   const Server *server) const;
	bool _isMethodAllowed(const std::string &method,
						  const Location *location) const;
	void _generateErrorResponse(HttpResponse &response,
								int statusCode,
								const Server *server) const;

public:
	RequestRouter();
	~RequestRouter();

	void route(const HttpRequest &request,
			   HttpResponse &response,
			   const Server *server);
};

#endif
