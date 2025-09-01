#ifndef IMETHODHANDLER_HPP
#define IMETHODHANDLER_HPP

#include <string>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Server.hpp"
#include "Location.hpp"

class IMethodHandler
{
public:
	virtual ~IMethodHandler() {}

	// Main handler method
	virtual void handle(const HttpRequest &request,
						HttpResponse &response,
						const Server *server,
						const Location *location = NULL) = 0;

	// Check if this handler can process the request
	virtual bool canHandle(const std::string &method) const = 0;

	// Get supported method name
	virtual std::string getMethod() const = 0;

protected:
	// Common utility methods that all handlers can use
	static std::string resolveFilePath(const std::string &uri,
									   const Server *server,
									   const Location *location);

	static std::string getMimeType(const std::string &filePath);

	static bool isPathAccessible(const std::string &path);

	static std::string readFile(const std::string &filePath);

	static void setCommonHeaders(HttpResponse &response,
								 const std::string &contentType,
								 size_t contentLength);
};

#endif /* IMETHODHANDLER_HPP */
