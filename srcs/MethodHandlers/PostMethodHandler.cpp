#include "../../includes/Core/PostMethodHandler.hpp"
#include <sys/stat.h>
#include <unistd.h>

PostMethodHandler::PostMethodHandler()
{
}

PostMethodHandler::PostMethodHandler(const PostMethodHandler &other)
{
	*this = other;
}

PostMethodHandler::~PostMethodHandler()
{
}

PostMethodHandler &PostMethodHandler::operator=(const PostMethodHandler &other)
{
	(void)other;
	return *this;
}

bool PostMethodHandler::handleRequest(const HttpRequest &request, HttpResponse &response, const Server *server,
									  const Location *location)
{
	if (!canHandle(request.getMethod()))
	{
		response.setResponseDefaultBody(405, "Method Not Allowed", server, location, HttpResponse::ERROR);
		return false;
	}

	Logger::debug("PostMethodHandler: Processing POST request to: " + request.getUri(), __FILE__, __LINE__,
				  __PRETTY_FUNCTION__);

	// Check if it's a CGI request
	if (isCgiRequest(request.getUri(), location))
	{
		return handleCgiRequest(request, response, server, location);
	}
	else
	{
		// Handle as file upload
		return handleFileUpload(request, response, server, location);
	}
}

bool PostMethodHandler::canHandle(const std::string &method) const
{
	return method == "POST";
}

bool PostMethodHandler::handleCgiRequest(const HttpRequest &request, HttpResponse &response, const Server *server,
										 const Location *location)
{
	Logger::debug("PostMethodHandler: Handling CGI request", __FILE__, __LINE__, __PRETTY_FUNCTION__);

	// Check for internal redirect loop
	if (request.getInternalRedirectDepth() >= 5) // MAX_INTERNAL_REDIRECTS from HttpRequest
	{
		Logger::error("PostMethodHandler: Internal redirect loop detected (depth exceeded)", __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);
		response.setResponseDefaultBody(500, "Internal Redirect Loop Detected", server, location, HttpResponse::ERROR);
		return false;
	}

	CgiHandler cgiHandler;
	CgiHandler::ExecutionResult result = cgiHandler.execute(request, response, server, location);

	// Check for internal redirect
	if (result == CgiHandler::SUCCESS && cgiHandler.isInternalRedirect())
	{
		std::string redirectPath = cgiHandler.getInternalRedirectPath();
		Logger::info("PostMethodHandler: Processing internal redirect to: " + redirectPath, __FILE__, __LINE__,
					 __PRETTY_FUNCTION__);

		// Create a modified request for the redirect
		HttpRequest redirectRequest = request;
		redirectRequest.incrementInternalRedirectDepth();

		// Find the location for the redirect path
		const Location *newLocation = server->getLocation(redirectPath);
		if (!newLocation)
		{
			Logger::error("PostMethodHandler: No location found for internal redirect: " + redirectPath, __FILE__,
						  __LINE__, __PRETTY_FUNCTION__);
			response.setResponseDefaultBody(500, "Internal Redirect Failed", server, location, HttpResponse::ERROR);
			return false;
		}

		// For internal redirect, we need to re-process as a GET request to the new path
		// This is a simplified approach - you might need to adjust based on your architecture
		// The proper way would be to call the appropriate handler for the redirect path

		// For now, just log and return an error since full re-routing requires
		// access to the handler factory which we don't have in this context
		Logger::warning("PostMethodHandler: Internal redirect detected but full re-routing not implemented. "
						"Treating as external redirect instead.",
						__FILE__, __LINE__, __PRETTY_FUNCTION__);

		// Fall through to populate the response with redirect headers
		// The client will follow the redirect
	}

	switch (result)
	{
	case CgiHandler::SUCCESS:
		Logger::debug("PostMethodHandler: CGI execution successful", __FILE__, __LINE__, __PRETTY_FUNCTION__);
		return true;
	case CgiHandler::ERROR_INVALID_SCRIPT_PATH:
		response.setResponseDefaultBody(404, "Not Found", server, location, HttpResponse::ERROR);
		break;
	case CgiHandler::ERROR_SCRIPT_NOT_FOUND:
		response.setResponseDefaultBody(404, "Not Found", server, location, HttpResponse::ERROR);
		break;
	case CgiHandler::ERROR_EXECUTION_FAILED:
		response.setResponseDefaultBody(500, "Internal Server Error", server, location, HttpResponse::ERROR);
		break;
	case CgiHandler::ERROR_RESPONSE_PARSING_FAILED:
		response.setResponseDefaultBody(500, "Internal Server Error", server, location, HttpResponse::ERROR);
		break;
	case CgiHandler::ERROR_TIMEOUT:
		response.setResponseDefaultBody(504, "Gateway Timeout", server, location, HttpResponse::ERROR);
		break;
	default:
		response.setResponseDefaultBody(500, "Internal Server Error", server, location, HttpResponse::ERROR);
		break;
	}

	return false;
}

bool PostMethodHandler::handleFileUpload(const HttpRequest &request, HttpResponse &response, const Server *server,
										 const Location *location)
{
	Logger::debug("PostMethodHandler: Handling file upload", __FILE__, __LINE__, __PRETTY_FUNCTION__);

	// Get upload path
	std::string uploadPath = getUploadPath(server, location);
	if (uploadPath.empty())
	{
		response.setResponseDefaultBody(500, "Internal Server Error", server, location, HttpResponse::ERROR);
		return false;
	}

	// Generate unique filename
	std::string filename = "upload_" + StrUtils::toString(time(NULL)) + ".dat";
	std::string filePath = uploadPath + "/" + filename;

	// Save the uploaded content
	if (!saveUploadedFile(filePath, request.getBodyData()))
	{
		response.setResponseDefaultBody(500, "Internal Server Error", server, location, HttpResponse::ERROR);
		return false;
	}

	// Return success response
	response.setResponseCustomBody(201, "Created", "File uploaded successfully: " + filename, "text/plain",
								   HttpResponse::SUCCESS);

	Logger::debug("PostMethodHandler: File uploaded successfully: " + filePath, __FILE__, __LINE__,
				  __PRETTY_FUNCTION__);
	return true;
}

bool PostMethodHandler::isCgiRequest(const std::string &uri, const Location *location)
{
	if (!location->hasCgiPath())
		return false;

	// Simple check: if URI contains .php, .py, .cgi, etc.
	size_t dotPos = uri.find_last_of('.');
	if (dotPos == std::string::npos)
		return false;

	std::string extension = uri.substr(dotPos + 1);
	StrUtils::toLowerCase(extension);

	return (extension == "php" || extension == "py" || extension == "cgi" || extension == "pl" || extension == "sh");
}

std::string PostMethodHandler::getUploadPath(const Server *server, const Location *location)
{
	(void)location;
	// For now, use a simple upload directory
	// In a real implementation, this would be configurable
	std::string uploadDir = server->getRootPath() + "/uploads";

	// Create upload directory if it doesn't exist
	struct stat st;
	if (stat(uploadDir.c_str(), &st) != 0)
	{
		if (mkdir(uploadDir.c_str(), 0755) != 0)
		{
			Logger::error("PostMethodHandler: Failed to create upload directory: " + uploadDir, __FILE__, __LINE__,
						  __PRETTY_FUNCTION__);
			return "";
		}
	}

	return uploadDir;
}

bool PostMethodHandler::saveUploadedFile(const std::string &filePath, const std::string &content)
{
	std::ofstream file(filePath.c_str(), std::ios::binary);
	if (!file.is_open())
	{
		Logger::error("PostMethodHandler: Failed to open file for writing: " + filePath, __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);
		return false;
	}

	file.write(content.c_str(), content.length());
	file.close();

	return true;
}
