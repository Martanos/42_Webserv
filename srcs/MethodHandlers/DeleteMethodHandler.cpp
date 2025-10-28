#include "../../includes/Core/DeleteMethodHandler.hpp"

DeleteMethodHandler::DeleteMethodHandler()
{
}

DeleteMethodHandler::DeleteMethodHandler(const DeleteMethodHandler &other)
{
	*this = other;
}

DeleteMethodHandler::~DeleteMethodHandler()
{
}

DeleteMethodHandler &DeleteMethodHandler::operator=(const DeleteMethodHandler &other)
{
	(void)other;
	return *this;
}

bool DeleteMethodHandler::handleRequest(const HttpRequest &request, HttpResponse &response, 
									   const Server *server, const Location *location)
{
	if (!canHandle(request.getMethod()))
	{
		response.setStatus(405, "Method Not Allowed");
		return false;
	}

	Logger::debug("DeleteMethodHandler: Processing DELETE request to: " + request.getUri());

	// Get the file path
	std::string filePath = getFilePath(request.getUri(), server, location);
	if (filePath.empty())
	{
		response.setStatus(400, "Bad Request");
		return false;
	}

	// Check if it's safe to delete
	if (!isSafeToDelete(filePath, server, location))
	{
		response.setStatus(403, "Forbidden");
		return false;
	}

	// Check if file exists
	struct stat st;
	if (stat(filePath.c_str(), &st) != 0)
	{
		response.setStatus(404, "Not Found");
		return false;
	}

	// Check if it's a regular file
	if (!S_ISREG(st.st_mode))
	{
		response.setStatus(403, "Forbidden");
		return false;
	}

	// Delete the file
	return deleteFile(filePath, response);
}

bool DeleteMethodHandler::canHandle(const std::string &method) const
{
	return method == "DELETE";
}

bool DeleteMethodHandler::deleteFile(const std::string &filePath, HttpResponse &response)
{
	if (unlink(filePath.c_str()) != 0)
	{
		Logger::error("DeleteMethodHandler: Failed to delete file: " + filePath);
		response.setStatus(500, "Internal Server Error");
		return false;
	}

	Logger::debug("DeleteMethodHandler: Successfully deleted file: " + filePath);
	response.setStatus(200, "OK");
	response.setBody("File deleted successfully");
	response.setHeader(Header("Content-Type: text/plain"));
	return true;
}

bool DeleteMethodHandler::isSafeToDelete(const std::string &filePath, const Server *server, const Location *location)
{
	// Check if the file is within the server's root directory
	std::string rootPath = location->hasRoot() ? location->getRoot() : server->getRootPath();
	
	// Ensure the file path starts with the root path
	if (filePath.find(rootPath) != 0)
	{
		Logger::warning("DeleteMethodHandler: Attempted to delete file outside root: " + filePath);
		return false;
	}

	// Check for path traversal attempts
	if (filePath.find("..") != std::string::npos)
	{
		Logger::warning("DeleteMethodHandler: Path traversal attempt detected: " + filePath);
		return false;
	}

	return true;
}

std::string DeleteMethodHandler::getFilePath(const std::string &uri, const Server *server, const Location *location)
{
	std::string rootPath = location->hasRoot() ? location->getRoot() : server->getRootPath();
	
	// Sanitize the URI to prevent path traversal
	std::string sanitizedUri = StrUtils::sanitizeUriPath(uri);
	
	// Combine root path with sanitized URI
	std::string filePath = rootPath + sanitizedUri;
	
	// Normalize the path
	filePath = StrUtils::normalizeSlashes(filePath);
	
	return filePath;
}