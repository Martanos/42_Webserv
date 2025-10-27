#ifndef DELETEHANDLER_HPP
#define DELETEHANDLER_HPP

#include "../../includes/Core/Location.hpp"
#include "../../includes/Core/Server.hpp"
#include "../../includes/HTTP/HttpRequest.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

namespace DELETE
{

namespace DELETE_UTILS
{
static inline bool isDirectoryEmpty(const std::string &dirPath)
{

	struct dirent *entry;
	DIR *dir = opendir(dirPath.c_str());
	if (dir == NULL)
	{
		return false;
	}
	closedir(dir);
	return true;
}
} // namespace DELETE_UTILS
// Deletion method utilities
static inline void handler(const HttpRequest &request, HttpResponse &response, const Server *server,
						   const Location *location)
{
	std::string filePath = request.getUri();

	// 1. Get file statistics
	struct stat fileStat;
	if (stat(filePath.c_str(), &fileStat) != 0)
		return response.setResponse(404, "Not Found", server, location, filePath);

	// 2. Check with access if file is accessible
	if (access(filePath.c_str(), W_OK) != 0)
		return response.setResponse(403, "Forbidden", server, location, filePath);

	// 3. If its a directory, check if it is empty
	if (S_ISDIR(fileStat.st_mode))
	{
		// Check if the directory is empty
		if (DELETE_UTILS::isDirectoryEmpty(filePath))
		{
			// Delete the directory
			if (rmdir(filePath.c_str()) != 0)
				return response.setResponse(500, "Internal Server Error", server, location, filePath);
			return response.setResponse(200, "OK", server, location, filePath);
		}
		else
			return response.setResponse(403, "Forbidden", server, location, filePath);
	}
	else if (S_ISREG(fileStat.st_mode))
	{
		// Delete the file
		if (unlink(filePath.c_str()) != 0)
			return response.setResponse(500, "Internal Server Error", server, location, filePath);
		return response.setResponse(200, "OK", server, location, filePath);
	}
	else
		return response.setResponse(501, "Not Implemented", server, location, filePath);
}

} // namespace DELETE

#endif