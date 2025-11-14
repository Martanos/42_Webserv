#ifndef DELETEMETHODHANDLER_HPP
#define DELETEMETHODHANDLER_HPP

#include "IMethodHandler.hpp"
#include <sys/stat.h>
#include <unistd.h>

class DeleteMethodHandler : public IMethodHandler
{
public:
	DeleteMethodHandler();
	DeleteMethodHandler(const DeleteMethodHandler &other);
	~DeleteMethodHandler();
	DeleteMethodHandler &operator=(const DeleteMethodHandler &other);

	// IMethodHandler implementation
	virtual bool handleRequest(const HttpRequest &request, HttpResponse &response, const Server *server,
							   const Location *location);
	virtual bool canHandle(const std::string &method) const;

private:
	// Helper methods
	bool deleteFile(const std::string &filePath, HttpResponse &response, const Server *server,
					const Location *location);
	bool isSafeToDelete(const std::string &filePath, const Server *server, const Location *location);
	std::string getFilePath(const std::string &uri, const Server *server, const Location *location);
};

#endif /* DELETEMETHODHANDLER_HPP */
