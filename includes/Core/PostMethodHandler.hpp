#ifndef POSTMETHODHANDLER_HPP
#define POSTMETHODHANDLER_HPP

#include "IMethodHandler.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/CGI/CgiHandler.hpp"
#include <fstream>
#include <sstream>

class PostMethodHandler : public IMethodHandler
{
public:
	PostMethodHandler();
	PostMethodHandler(const PostMethodHandler &other);
	~PostMethodHandler();
	PostMethodHandler &operator=(const PostMethodHandler &other);

	// IMethodHandler implementation
	virtual bool handleRequest(const HttpRequest &request, HttpResponse &response, 
							  const Server *server, const Location *location);
	virtual bool canHandle(const std::string &method) const;

private:
	// Helper methods
	bool handleCgiRequest(const HttpRequest &request, HttpResponse &response, 
						  const Server *server, const Location *location);
	bool handleFileUpload(const HttpRequest &request, HttpResponse &response, 
						  const Server *server, const Location *location);
	bool isCgiRequest(const std::string &uri, const Location *location);
	std::string getUploadPath(const Server *server, const Location *location);
	bool saveUploadedFile(const std::string &filePath, const std::string &content);
};

#endif /* POSTMETHODHANDLER_HPP */