#include "../../includes/MethodHandlers/PostMethodHandler.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/MimeTypeResolver.hpp"
#include "../../includes/Utils/FileUtils.hpp"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

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
	return (*this);
}

bool PostMethodHandler::handleRequest(const HttpRequest &request, HttpResponse &response, const Location &location)
{
	Logger::debug("PostMethodHandler: Processing POST request to: " + request.getURI().getResolvedPath(), __FILE__,
				  __LINE__, __PRETTY_FUNCTION__);

	// Check if CGI - POST is commonly used for CGI form submissions
	if (location.getLocationType() == Location::CGI)
	{
		return executeCgi(request.getURI().getResolvedPath(), request, response, location);
	}

	// Check if requested file is executable - execute as CGI for POST requests
	// This handles cases where executable files (like CGI binaries) are posted to
	std::string filePath = request.getURI().getResolvedPath();
	if (FileUtils::isFileExecutable(filePath))
	{
		Logger::debug("PostMethodHandler: File is executable, treating as CGI: " + filePath, __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);
		return executeCgi(filePath, request, response, location);
	}

	// Handle file upload
	Logger::debug("PostMethodHandler: Handling file upload", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	// Get upload path
	if (!_validateUploadPath(location, response))
		return (false);
	// Generate unique filename
	std::string uploadFilePath;
	if (!_generateUniqueFilename(*location.getUploadPath(), request, uploadFilePath))
	{
		response.setResponseDefaultBody(500, "Internal Server Error: Failed to generate unique filename", &location,
										HttpResponse::ERROR);
		return (false);
	}
	// Save the uploaded content
	if (!_saveUploadedFile(uploadFilePath, request))
	{
		response.setResponseDefaultBody(500, "Could not save uploaded file", &location, HttpResponse::ERROR);
		return (false);
	}
	// Return success response
	response.setResponseCustomBody(201, "Created", "File uploaded successfully: " + uploadFilePath, "text/plain",
								   HttpResponse::SUCCESS);
	Logger::debug("PostMethodHandler: File uploaded successfully: " + uploadFilePath, __FILE__, __LINE__,
				  __PRETTY_FUNCTION__);
	return (true);
}

bool PostMethodHandler::_validateUploadPath(const Location &location, HttpResponse &response)
{
	struct stat pathStat;

	// Check if upload path is set in location
	if (location.getLocationType() != Location::UPLOAD)
	{
		response.setResponseDefaultBody(500, "Location does not allow file uploads", &location, HttpResponse::ERROR);
		return (false);
	}
	const std::string *uploadPath = location.getUploadPath();
	// Sanitize upload path
	if (uploadPath->empty())
	{
		response.setResponseDefaultBody(500, "Upload path is empty", &location, HttpResponse::ERROR);
		return (false);
	}
	// Check if upload path exists and is a directory
	if (stat(uploadPath->c_str(), &pathStat) != 0 || !S_ISDIR(pathStat.st_mode))
	{
		response.setResponseDefaultBody(500, "Upload path does not exist or is not a directory", &location,
										HttpResponse::ERROR);
		return (false);
	}
	return (true);
}

bool PostMethodHandler::_generateUniqueFilename(const std::string &uploadPath, const HttpRequest &request,
												std::string &filePath)
{
	// Extract filename from Content-Disposition header if present
	const Header *contentTypeHeader = request.getHeaders().getHeader("content-type");
	if (!contentTypeHeader || contentTypeHeader->getValues().empty())
	{
		filePath = FileUtils::generateFileName(uploadPath, "", "");
		if (filePath.empty())
		{
			return (false);
		}
		return (true);
	}
	std::vector<std::string> headerValues = contentTypeHeader->getValues();
	std::string extenstion = MimeTypeResolver::getExtensionByMimeType(headerValues[0]);
	filePath = FileUtils::generateFileName(uploadPath, "", extenstion);
	if (filePath.empty())
	{
		return (false);
	}
	return (true);
}

bool PostMethodHandler::_saveUploadedFile(const std::string &filePath, const HttpRequest &request)
{
	try
	{
		// Split logic paths into temp file or in memory based on body storage
		if (request.getBody().getIsUsingTempFile())
		{
			HttpRequest &mutableRequest = const_cast<HttpRequest &>(request);
			HttpBody &mutableBody = mutableRequest.getBody();
			FileManager &tempFile = mutableBody.getTempFileManager();
			if (!tempFile.moveTo(filePath, false))
			{
				Logger::error("PostMethodHandler: Failed to move temp file from " + tempFile.getFilePath() + " to " +
								  filePath + ": " + std::string(strerror(errno)),
							  __FILE__, __LINE__, __PRETTY_FUNCTION__);
				return false;
			}
		}
		else
		{
			if (!FileUtils::writeToFile(filePath, request.getBody().getRawBody()))
			{
				Logger::error("PostMethodHandler: Failed to write uploaded file: " + std::string(strerror(errno)),
							  __FILE__, __LINE__, __PRETTY_FUNCTION__);
				return false;
			}
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("PostMethodHandler: Failed to save uploaded file: " + std::string(e.what()), __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);
		return (false);
	}
	return (true);
}
