#ifndef GETMETHODHANDLER_HPP
#define GETMETHODHANDLER_HPP

#include <vector>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include "HttpResponse.hpp"
#include "Server.hpp"
#include "Location.hpp"
#include "HttpRequest.hpp"
#include "MimeTypes.hpp"
#include "Logger.hpp"
#include "DefaultStatusMap.hpp"
#include "StringUtils.hpp"
#include "IMethodHandler.hpp"

class GetMethodHandler : public IMethodHandler
{
private:
	// Private methods for GET-specific operations
	void serveFile(const std::string &filePath, HttpResponse &response) const;

	void generateDirectoryListing(const std::string &dirPath,
								  const std::string &uri,
								  HttpResponse &response,
								  const Server *server,
								  const Location *location) const;

	bool tryIndexFiles(const std::string &dirPath,
					   const std::vector<std::string> &indexes,
					   HttpResponse &response) const;

	std::string formatFileSize(off_t size) const;

	std::string urlEncode(const std::string &str) const;

	bool isHiddenFile(const std::string &filename) const;

public:
	// Orthodox Canonical Form
	GetMethodHandler();
	GetMethodHandler(const GetMethodHandler &src);
	virtual ~GetMethodHandler();
	GetMethodHandler &operator=(const GetMethodHandler &rhs);

	// IMethodHandler interface implementation
	virtual void handle(const HttpRequest &request,
						HttpResponse &response,
						const Server *server,
						const Location *location = NULL);

	virtual bool canHandle(const std::string &method) const;

	virtual std::string getMethod() const;
};

#endif /* GETMETHODHANDLER_HPP */
