#ifndef GETMETHODHANDLER_HPP
#define GETMETHODHANDLER_HPP

#include "IMethodHandler.hpp"
#include "../../includes/Cgi/CgiHandler.hpp"
#include <sys/stat.h>

class GetMethodHandler : public IMethodHandler
{
private:
	// Utils
	std::string _generateDirectoryListing(const std::string &filePath);

	// File type helper methods
	bool _serveFile(const HttpRequest &request, const std::string &filePath, HttpResponse &response, const Location &location);
	bool _serveDirectory(const HttpRequest &request, HttpResponse &response, const Location &location);
	bool _serveSymlink(const HttpRequest &request, const std::string &linkPath, HttpResponse &response, const Location &location);

public:
	GetMethodHandler();
	GetMethodHandler(const GetMethodHandler &other);
	~GetMethodHandler();
	GetMethodHandler &operator=(const GetMethodHandler &other);

	// IMethodHandler implementation
	virtual bool handleRequest(const HttpRequest &request, HttpResponse &response, const Location &location);
};

#endif /* GETMETHODHANDLER_HPP */
