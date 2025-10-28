// PostMethodHandler.hpp
#ifndef POSTMETHODHANDLER_HPP
#define POSTMETHODHANDLER_HPP

#include "../../includes/CGI/CgiHandler.hpp"
#include "../../includes/Core/Location.hpp"
#include "../../includes/Core/Server.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/HTTP/HttpRequest.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <vector>

namespace POST
{
// Nested namespace for POST utilities
namespace POST_UTILS
{

static inline std::string generateSafeFilename(const std::string &originalFilename)
{
	std::string safeFilename = originalFilename;

	// Remove or replace dangerous characters
	for (size_t i = 0; i < safeFilename.length(); ++i)
	{
		char c = safeFilename[i];
		if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
		{
			safeFilename[i] = '_';
		}
	}

	// Add timestamp to make filename unique
	std::time_t now = std::time(0);
	std::stringstream ss;
	ss << std::time(&now) << "_" << safeFilename;

	return ss.str();
}

static inline void handleCGI(const HttpRequest &request, HttpResponse &response, const Location *location,
							 const Server *server)
{
	// Use the new CgiHandler for CGI execution
	CgiHandler cgiHandler;

	CgiHandler::ExecutionResult result = cgiHandler.execute(request, response, server, location);

	if (result != CgiHandler::SUCCESS)
	{
		// CGI execution failed, return appropriate error
		int statusCode = 500;
		std::string statusMessage = "Internal Server Error";

		switch (result)
		{
		case CgiHandler::ERROR_SCRIPT_NOT_FOUND:
			statusCode = 404;
			statusMessage = "Not Found";
			break;
		case CgiHandler::ERROR_TIMEOUT:
			statusCode = 504;
			statusMessage = "Gateway Timeout";
			break;
		case CgiHandler::ERROR_INVALID_SCRIPT_PATH:
			statusCode = 400;
			statusMessage = "Bad Request";
			break;
		default:
			statusCode = 500;
			statusMessage = "Internal Server Error";
			break;
		}

		response.setStatus(statusCode, statusMessage);
		response.setBody(server->getStatusPage(statusCode));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
	}
}

static inline std::string extractFilename(const std::string &contentDisposition)
{
	size_t pos = contentDisposition.find("filename=\"");
	if (pos != std::string::npos)
	{
		pos += 10; // Length of "filename=\""
		size_t endPos = contentDisposition.find("\"", pos);
		if (endPos != std::string::npos)
		{
			return contentDisposition.substr(pos, endPos - pos);
		}
	}
	return "";
}

static inline bool saveUploadedFile(const std::string &uploadPath, const std::string &filename,
									const std::string &content)
{
	std::string fullPath = uploadPath;
	if (fullPath[fullPath.length() - 1] != '/')
	{
		fullPath += '/';
	}

	// Generate robust filename with timestamp and random component
	std::string safeFilename = generateSafeFilename(filename);
	fullPath += safeFilename;

	// Use FileDescriptor for safe file handling
	FileDescriptor fd = FileDescriptor::createFromOpen(fullPath.c_str(), O_WRONLY | O_CREAT | O_EXCL,
													   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (!fd.isValid())
	{
		// Try with a numbered suffix if file exists
		for (int i = 1; i < 100; ++i)
		{
			std::stringstream newPath;
			size_t dotPos = filename.find_last_of('.');
			if (dotPos != std::string::npos)
			{
				newPath << uploadPath << "/" << filename.substr(0, dotPos) << "_" << i << filename.substr(dotPos);
			}
			else
			{
				newPath << uploadPath << "/" << filename << "_" << i;
			}

			fd = FileDescriptor::createFromOpen(newPath.str().c_str(), O_WRONLY | O_CREAT | O_EXCL,
												S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (fd.isValid())
			{
				break;
			}
		}

		if (!fd.isValid())
		{
			Logger::log(Logger::ERROR, "Cannot create upload file: " + fullPath);
			return false;
		}
	}

	// Write content
	ssize_t written = write(fd.getFd(), content.c_str(), content.length());
	if (written < 0 || static_cast<size_t>(written) != content.length())
	{
		Logger::log(Logger::ERROR, "Failed to write complete file: " + fullPath);
		return false;
	}

	// FileDescriptor destructor will close the file
	return true;
}
static inline void handleFileUpload(const HttpRequest &request, HttpResponse &response, const Location *location,
									const Server *server)
{
	std::string uploadPath = location->getRoot();

	// Verify upload directory exists and is writable
	struct stat st;
	if (stat(uploadPath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
	{
		Logger::log(Logger::ERROR, "Upload directory not accessible: " + uploadPath);
		response.setStatus(500, "Internal Server Error");
		response.setBody(location, server);
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StrUtils::toString(response.getBody().length()));
		return;
	}

	// The body is already parsed, just save it
	// Generate filename based on timestamp if not provided
	std::stringstream ss;
	ss << "upload_" << time(NULL) << "_" << rand() % 10000;

	// Check Content-Type for filename hint
	std::string contentType = request.getHeader("content-type")[0];
	std::string filename = ss.str();

	// If it's a form upload, try to extract filename from Content-Disposition
	std::string contentDisposition = request.getHeader("content-disposition")[0];
	if (!contentDisposition.empty())
	{
		std::string extractedName = extractFilename(contentDisposition);
		if (!extractedName.empty())
		{
			filename = extractedName;
		}
	}
	else if (contentType.find("application/octet-stream") != std::string::npos)
	{
		filename += ".bin";
	}
	else if (contentType.find("text/plain") != std::string::npos)
	{
		filename += ".txt";
	}
	else if (contentType.find("image/") != std::string::npos)
	{
		size_t pos = contentType.find("image/");
		std::string ext = contentType.substr(pos + 6);
		size_t semicolon = ext.find(';');
		if (semicolon != std::string::npos)
		{
			ext = ext.substr(0, semicolon);
		}
		filename += "." + ext;
	}
	else
	{
		filename += ".dat";
	}

	// Save the file
	if (saveUploadedFile(uploadPath, filename, request.getBodyData()))
	{
		// Success response
		std::stringstream html;
		html << "<!DOCTYPE html>\n<html>\n<head>\n";
		html << "<title>Upload Successful</title>\n</head>\n<body>\n";
		html << "<h1>File Uploaded Successfully</h1>\n";
		html << "<p>Filename: " << filename << "</p>\n";
		html << "<p>Size: " << request.getBodyData().length() << " bytes</p>\n";
		html << "<p><a href=\"" << request.getUri() << "\">Back</a></p>\n";
		html << "</body>\n</html>\n";

		response.setStatus(201, "Created");
		response.setBody(html.str());
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
		response.setHeader("Location", uploadPath + "/" + filename);
	}
	else
	{
		// Failed to save
		response.setStatus(500, "Internal Server Error");
		response.setBody(server->getStatusPage(500));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
	}
}

} // namespace POST_UTILS

static inline void handle(const HttpRequest &request, HttpResponse &response, const Server *server,
						  const Location *location)
{
	// Check what type of POST handling is configured
	if (location && !location->getCgiPath().empty())
	{
		handleCGI(request, response, location, server);
	}
	else if (location && !location->getUploadPath().empty())
	{
		handleFileUpload(request, response, location, server);
	}
	else
	{
		// No specific POST handler configured - return error
		response.setStatus(501, "Not Implemented");
		response.setBody(server->getStatusPage(501));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
	}
}
} // namespace POST

#endif /* POSTMETHODHANDLER_HPP */