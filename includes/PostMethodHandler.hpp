// PostMethodHandler.hpp
#ifndef POSTMETHODHANDLER_HPP
#define POSTMETHODHANDLER_HPP

#include "CgiHandler.hpp"
#include "FileDescriptor.hpp"
#include "IMethodHandler.hpp"
#include <string>

class PostMethodHandler : public IMethodHandler
{
private:
	// Since HttpRequest already parses the body, we just need to handle it
	void handleFileUpload(const HttpRequest &request, HttpResponse &response, const Location *location,
						  const Server *server) const;

	// CGI handling
	void handleCGI(const HttpRequest &request, HttpResponse &response, const Location *location,
				   const Server *server) const;

	// Save uploaded content to file
	bool saveUploadedFile(const std::string &uploadPath, const std::string &filename, const std::string &content) const;

	// Extract filename from multipart boundary (if needed)
	std::string extractFilename(const std::string &contentDisposition) const;

public:
	// Orthodox Canonical Form
	PostMethodHandler();
	PostMethodHandler(const PostMethodHandler &src);
	virtual ~PostMethodHandler();
	PostMethodHandler &operator=(const PostMethodHandler &rhs);

	// IMethodHandler interface
	virtual void handle(const HttpRequest &request, HttpResponse &response, const Server *server,
						const Location *location = NULL);

	virtual bool canHandle(const std::string &method) const;
	virtual std::string getMethod() const;
};

#endif /* *********************************************** POSTMETHODHANDLER_H                                          \
		*/
