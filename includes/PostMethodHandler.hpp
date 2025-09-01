#ifndef POSTMETHODHANDLER_HPP
#define POSTMETHODHANDLER_HPP

#include <string>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstddef>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <ctime>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdbool>
#include <cstdint>
#include <cstdbool>
#include <cstdint>
#include <cstdbool>
#include "IMethodHandler.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Server.hpp"
#include "Location.hpp"
#include "Logger.hpp"
#include "DefaultStatusMap.hpp"
#include "StringUtils.hpp"

class PostMethodHandler : public IMethodHandler
{
private:
	// File upload handling
	void handleFileUpload(const HttpRequest &request,
						  HttpResponse &response,
						  const Location *location,
						  const Server *server) const;

	// CGI handling
	void handleCGI(const HttpRequest &request,
				   HttpResponse &response,
				   const Location *location,
				   const Server *server) const;

	// Parse multipart form data
	bool parseMultipartData(const std::string &body,
							const std::string &boundary,
							std::string &filename,
							std::string &fileContent) const;

	// Validate content length
	bool validateContentLength(size_t contentLength,
							   const Server *server) const;

	// Generate unique filename
	std::string generateUniqueFilename(const std::string &uploadPath,
									   const std::string &originalName) const;

	// Extract boundary from Content-Type header
	std::string extractBoundary(const std::string &contentType) const;

public:
	// Orthodox Canonical Form
	PostMethodHandler();
	PostMethodHandler(const PostMethodHandler &src);
	virtual ~PostMethodHandler();
	PostMethodHandler &operator=(const PostMethodHandler &rhs);

	// IMethodHandler interface implementation
	virtual void handle(const HttpRequest &request,
						HttpResponse &response,
						const Server *server,
						const Location *location = NULL);

	virtual bool canHandle(const std::string &method) const;

	virtual std::string getMethod() const;
};

#endif /* *********************************************** POSTMETHODHANDLER_H */
