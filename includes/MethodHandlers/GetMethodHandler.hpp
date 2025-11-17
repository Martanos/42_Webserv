#ifndef GETMETHODHANDLER_HPP
# define GETMETHODHANDLER_HPP

# include "IMethodHandler.hpp"
# include <sys/stat.h>

class GetMethodHandler : public IMethodHandler
{
  public:
	GetMethodHandler();
	GetMethodHandler(const GetMethodHandler &other);
	~GetMethodHandler();
	GetMethodHandler &operator=(const GetMethodHandler &other);

	// IMethodHandler implementation
	virtual bool handleRequest(const HttpRequest &request,
		HttpResponse &response, const Location &location);

  private:
	// Helper methods
	bool serveFile(const std::string &filePath, HttpResponse &response,
		const Location &location);
	bool serveDirectory(const std::string &dirPath, HttpResponse &response,
		const Location &location);
	std::string generateDirectoryListing(const std::string &filePath);
};

#endif /* GETMETHODHANDLER_HPP */
