#ifndef DELETEMETHODHANDLER_HPP
#define DELETEMETHODHANDLER_HPP

#include "IMethodHandler.hpp"
#include <sys/stat.h>
#include <unistd.h>

class DeleteMethodHandler : public IMethodHandler
{
private:
	// Helper methods
	bool deleteFile(const std::string &filePath, HttpResponse &response, const Location *location);

public:
	DeleteMethodHandler();
	DeleteMethodHandler(const DeleteMethodHandler &other);
	~DeleteMethodHandler();
	DeleteMethodHandler &operator=(const DeleteMethodHandler &other);

	// IMethodHandler implementation
	virtual bool handleRequest(const HttpRequest &request, HttpResponse &response, const Location *location);
};

#endif /* DELETEMETHODHANDLER_HPP */
