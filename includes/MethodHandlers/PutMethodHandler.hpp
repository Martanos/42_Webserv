#ifndef PUTMETHODHANDLER_HPP
#define PUTMETHODHANDLER_HPP

#include "IMethodHandler.hpp"
#include <string>

class PutMethodHandler : public IMethodHandler
{
public:
	PutMethodHandler();
	PutMethodHandler(const PutMethodHandler &other);
	~PutMethodHandler();
	PutMethodHandler &operator=(const PutMethodHandler &other);

	virtual bool handleRequest(const HttpRequest &request, HttpResponse &response, const Server *server,
							   const Location *location);
	virtual bool canHandle(const std::string &method) const;

private:
	bool _ensureDirectory(const std::string &filePath) const;
	bool _writeBodyToFile(const HttpRequest &request, const std::string &filePath) const;
};

#endif /* PUTMETHODHANDLER_HPP */
