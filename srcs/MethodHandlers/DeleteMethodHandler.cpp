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

bool DeleteMethodHandler::handleRequest(const HttpRequest &request, HttpResponse &response, const Server *server,
										const Location *location)
{
	if (!canHandle(request.getMethod()))
	{
		response.setResponseDefaultBody(405, "Method Not Allowed", server, location, HttpResponse::ERROR);
		return false;
	}

	// Get the sanitized file path from the request
	std::string filePath = request.getUri();

	Logger::debug("DeleteMethodHandler: Processing DELETE request to: " + filePath, __FILE__, __LINE__,
				  __PRETTY_FUNCTION__);

	// Check if it's safe to delete
	if (!isSafeToDelete(filePath, server, location))
	{
		response.setResponseDefaultBody(403, "Forbidden", server, location, HttpResponse::ERROR);
		return false;
	}

	// Check if file exists
	struct stat st;
	if (stat(filePath.c_str(), &st) != 0)
	{
		response.setResponseDefaultBody(404, "File not found", server, location, HttpResponse::ERROR);
		return false;
	}

	// Check if it's a regular file
	if (!S_ISREG(st.st_mode))
	{
		response.setResponseDefaultBody(403, "Forbidden", server, location, HttpResponse::ERROR);
		return false;
	}

	// Delete the file
	return deleteFile(filePath, response, server, location);
}

bool DeleteMethodHandler::canHandle(const std::string &method) const
{
	return method == "DELETE";
}

bool DeleteMethodHandler::deleteFile(const std::string &filePath, HttpResponse &response, const Server *server,
									 const Location *location)
{
	if (unlink(filePath.c_str()) != 0)
	{
		Logger::error("DeleteMethodHandler: Failed to delete file: " + filePath, __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);
		response.setResponseDefaultBody(500, "Failed to delete file: " + filePath, server, location,
										HttpResponse::ERROR);
		return false;
	}

	Logger::debug("DeleteMethodHandler: Successfully deleted file: " + filePath, __FILE__, __LINE__,
				  __PRETTY_FUNCTION__);
	response.setResponseDefaultBody(200, "File deleted successfully", server, location, HttpResponse::SUCCESS);
	return true;
}

bool DeleteMethodHandler::isSafeToDelete(const std::string &filePath, const Server *server, const Location *location)
{
	// Check if the file is within the server's root directory
	std::string rootPath = location->hasRoot() ? location->getRoot() : server->getRootPath();

	// Ensure the file path starts with the root path
	if (filePath.find(rootPath) != 0)
	{
		Logger::warning("DeleteMethodHandler: Attempted to delete file outside root: " + filePath, __FILE__, __LINE__,
						__PRETTY_FUNCTION__);
		return false;
	}

	// Check for path traversal attempts
	if (filePath.find("..") != std::string::npos)
	{
		Logger::warning("DeleteMethodHandler: Path traversal attempt detected: " + filePath, __FILE__, __LINE__,
						__PRETTY_FUNCTION__);
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
