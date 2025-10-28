#ifndef DELETEMETHODHANDLER_HPP
#define DELETEMETHODHANDLER_HPP

#include "IMethodHandler.hpp"
#include "Logger.hpp"
#include <limits.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

class DeleteMethodHandler : public IMethodHandler
{
private:
	// Check if deletion is allowed for this path
	bool isDeletionAllowed(const std::string &filePath, const Location *location) const;

	// Safely delete a file
	bool deleteFile(const std::string &filePath) const;

public:
	// Orthodox Canonical Form
	DeleteMethodHandler();
	DeleteMethodHandler(const DeleteMethodHandler &src);
	virtual ~DeleteMethodHandler();
	DeleteMethodHandler &operator=(const DeleteMethodHandler &rhs);

	// IMethodHandler interface
	virtual void handle(const HttpRequest &request, HttpResponse &response, const Server *server,
						const Location *location = NULL);

	virtual bool canHandle(const std::string &method) const;
	virtual std::string getMethod() const;
};

#endif
