// includes/handlers/DeleteMethodHandler.hpp
#ifndef DELETEMETHODHANDLER_HPP
#define DELETEMETHODHANDLER_HPP

#include "IMethodHandler.hpp"

class DeleteMethodHandler : public IMethodHandler
{
private:
	bool _canDeleteFile(const std::string &filePath,
						const Location *location) const;

public:
	DeleteMethodHandler();
	virtual ~DeleteMethodHandler();

	virtual void handle(const HttpRequest &request,
						HttpResponse &response,
						const Server *server,
						const Location *location = NULL);

	virtual bool canHandle(const std::string &method) const;

	virtual std::string getMethod() const;
};

#endif
