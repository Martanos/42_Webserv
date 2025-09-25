#include "DeleteMethodHandler.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

DeleteMethodHandler::DeleteMethodHandler()
{
}

DeleteMethodHandler::DeleteMethodHandler(const DeleteMethodHandler &src)
{
	(void)src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

DeleteMethodHandler::~DeleteMethodHandler()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

DeleteMethodHandler &DeleteMethodHandler::operator=(DeleteMethodHandler const &rhs)
{
	(void)rhs;
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void DeleteMethodHandler::handle(const HttpRequest &request, HttpResponse &response, const Server *server,
								 const Location *location)
{
	// Check if DELETE is allowed at this location
	if (location)
	{
		const std::vector<std::string> &methods = location->getAllowedMethods();
		bool deleteAllowed = false;
		for (size_t i = 0; i < methods.size(); ++i)
		{
			if (methods[i] == "DELETE")
			{
				deleteAllowed = true;
				break;
			}
		}

		if (!deleteAllowed)
		{
			response.setStatus(405, "Method Not Allowed");
			response.setBody(server->getStatusPage(405));
			response.setHeader("Content-Type", "text/html");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
			response.setHeader("Allow", "GET, HEAD, POST");
			return;
		}
	}
	else
	{
		// No location config, DELETE not allowed by default
		response.setStatus(405, "Method Not Allowed");
		response.setBody(server->getStatusPage(405));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
		return;
	}

	// Resolve the file path
	std::string filePath = IMethodHandler::resolveFilePath(request.getUri(), server, location);

	// Check if file exists
	struct stat fileStat;
	if (stat(filePath.c_str(), &fileStat) != 0)
	{
		response.setStatus(404, "Not Found");
		response.setBody(server->getStatusPage(404));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
		return;
	}

	// Don't allow deletion of directories
	if (S_ISDIR(fileStat.st_mode))
	{
		response.setStatus(403, "Forbidden");
		response.setBody(server->getStatusPage(403));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
		return;
	}

	// Check if deletion is allowed for this specific path
	if (!isDeletionAllowed(filePath, location))
	{
		response.setStatus(403, "Forbidden");
		response.setBody(server->getStatusPage(403));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
		return;
	}

	// Attempt to delete the file
	if (deleteFile(filePath))
	{
		// Successful deletion
		std::stringstream html;
		html << "<!DOCTYPE html>\n<html>\n<head>\n";
		html << "<title>File Deleted</title>\n</head>\n<body>\n";
		html << "<h1>File Deleted Successfully</h1>\n";
		html << "<p>The requested resource has been deleted.</p>\n";
		html << "<p><a href=\"/\">Return to Home</a></p>\n";
		html << "</body>\n</html>\n";

		response.setStatus(200, "OK");
		response.setBody(html.str());
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
	}
	else
	{
		// Failed to delete
		response.setStatus(500, "Internal Server Error");
		response.setBody(server->getStatusPage(500));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
	}
}

bool DeleteMethodHandler::isDeletionAllowed(const std::string &filePath, const Location *location) const
{
	// Only allow deletion in upload directories
	if (!location || location->getUploadPath().empty())
	{
		return false;
	}

	// Check if the file is within the upload path
	std::string uploadPath = location->getUploadPath();

	// Resolve real paths to prevent directory traversal attacks
	char resolvedFile[PATH_MAX];
	char resolvedUpload[PATH_MAX];

	if (realpath(filePath.c_str(), resolvedFile) == NULL)
	{
		return false;
	}

	if (realpath(uploadPath.c_str(), resolvedUpload) == NULL)
	{
		return false;
	}

	// Check if file is within upload directory
	std::string fileStr(resolvedFile);
	std::string uploadStr(resolvedUpload);

	return fileStr.find(uploadStr) == 0;
}

bool DeleteMethodHandler::deleteFile(const std::string &filePath) const
{
	if (unlink(filePath.c_str()) == 0)
	{
		Logger::log(Logger::INFO, "File deleted: " + filePath);
		return true;
	}
	else
	{
		Logger::log(Logger::ERROR, "Failed to delete file: " + filePath);
		return false;
	}
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

bool DeleteMethodHandler::canHandle(const std::string &method) const
{
	return method == "DELETE";
}

std::string DeleteMethodHandler::getMethod() const
{
	return "DELETE";
}

/* ************************************************************************** */
