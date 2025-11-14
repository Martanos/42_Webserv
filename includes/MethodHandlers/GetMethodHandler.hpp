#ifndef GETMETHODHANDLER_HPP
#define GETMETHODHANDLER_HPP

#include "IMethodHandler.hpp"
#include <sys/stat.h>

class GetMethodHandler : public IMethodHandler
{
public:
	GetMethodHandler();
	GetMethodHandler(const GetMethodHandler &other);
	~GetMethodHandler();
	GetMethodHandler &operator=(const GetMethodHandler &other);

	// IMethodHandler implementation
	virtual bool handleRequest(const HttpRequest &request, HttpResponse &response, const Server *server,
							   const Location *location);
	virtual bool canHandle(const std::string &method) const;

private:
	// Helper methods
	bool serveFile(const std::string &filePath, HttpResponse &response, const Server *server, const Location *location);
	bool serveDirectory(const std::string &dirPath, HttpResponse &response, const Server *server,
						const Location *location);
	std::string generateDirectoryListing(const std::string &dirPath, const std::string &uri);
	bool isDirectory(const std::string &path);
	bool fileExists(const std::string &path);
};

#endif /* GETMETHODHANDLER_HPP */
