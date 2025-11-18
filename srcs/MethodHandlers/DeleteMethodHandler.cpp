#include "../../includes/MethodHandlers/DeleteMethodHandler.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Utils/FileUtils.hpp"

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

bool DeleteMethodHandler::handleRequest(const HttpRequest &request, HttpResponse &response, const Location *location)
{

	Logger::debug("DeleteMethodHandler: Processing DELETE request to: " + request.getURI().getResolvedPath(), __FILE__,
				  __LINE__, __PRETTY_FUNCTION__);

	// Logic splits between file, directory and others
	switch (FileUtils::getFileType(request.getURI().getResolvedPath()))
	{
	case FileUtils::REGULAR_FILE:
		return deleteFile(request.getURI().getResolvedPath(), response, location);
	case FileUtils::NOT_FOUND:
		response.setResponseDefaultBody(404, "Not Found: File does not exist", location, HttpResponse::ERROR);
		break;
	case FileUtils::DIRECTORY:
		response.setResponseDefaultBody(403, "Forbidden: Cannot delete directories", location, HttpResponse::ERROR);
		break;
	case FileUtils::SYMBOLIC_LINK:
		response.setResponseDefaultBody(403, "Forbidden: Cannot delete symbolic links", location, HttpResponse::ERROR);
		break;
	case FileUtils::CHARACTER_DEVICE:
		response.setResponseDefaultBody(403, "Forbidden: Cannot delete character devices", location,
										HttpResponse::ERROR);
		break;
	case FileUtils::BLOCK_DEVICE:
		response.setResponseDefaultBody(403, "Forbidden: Cannot delete block devices", location, HttpResponse::ERROR);
		break;
	case FileUtils::FIFO_PIPE:
		response.setResponseDefaultBody(403, "Forbidden: Cannot delete FIFO pipes", location, HttpResponse::ERROR);
		break;
	case FileUtils::SOCKET:
		response.setResponseDefaultBody(403, "Forbidden: Cannot delete socket files", location, HttpResponse::ERROR);
		break;
	default:
		response.setResponseDefaultBody(500, "Internal Server Error: Unknown file type", location, HttpResponse::ERROR);
		break;
	}
	return false;
}

bool DeleteMethodHandler::deleteFile(const std::string &filePath, HttpResponse &response, const Location *location)
{
	errno = 0;
	if (unlink(filePath.c_str()) != 0)
	{
		switch (errno)
		{
		case ENOENT:
			response.setResponseDefaultBody(404, "Not Found", location, HttpResponse::ERROR);
			break;
		case EACCES:
		case EPERM:
			response.setResponseDefaultBody(403, "Forbidden", location, HttpResponse::ERROR);
			break;
		case EISDIR:
			response.setResponseDefaultBody(405, "Method Not Allowed", location, HttpResponse::ERROR);
			break;
		default:
			response.setResponseDefaultBody(500, "Internal Server Error", location, HttpResponse::ERROR);
			break;
		}
		return false;
	}
	response.setResponseDefaultBody(204, "No Content", location, HttpResponse::SUCCESS);
	return true;
}
