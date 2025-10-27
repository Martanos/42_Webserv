#ifndef METHODHANDLERS_HPP
#define METHODHANDLERS_HPP

#include "../../includes/Core/Location.hpp"
#include "../../includes/Core/Server.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/MimeTypeResolver.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/HTTP/HttpRequest.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
#include <algorithm>
#include <dirent.h>
#include <iomanip>
#include <sys/stat.h>
#include <vector>



namespace POST
{
// Nested namespace for POST utilities
namespace POST_UTILS
{
static inline void handleFileUpload(const HttpRequest &request, HttpResponse &response, const Location *location,
									const Server *server)
{
	std::string uploadPath = location->getUploadPath();

	// Verify upload directory exists and is writable
	struct stat st;
	if (stat(uploadPath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
	{
		Logger::log(Logger::ERROR, "Upload directory not accessible: " + uploadPath);
		response.setStatus(500, "Internal Server Error");
		response.setBody(server->getStatusPage(500));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
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

namespace DELETE
{
// Deletion method utilities
static inline void handler(const HttpRequest &request, HttpResponse &response, const Server *server,
						   const Location *location)
{
	std::string filePath = request.getUri();

	// 1. Get file statistics
	struct stat fileStat;
	if (stat(filePath.c_str(), &fileStat) != 0)
	{
		response.setStatus(404, "Not Found");
		response.setBody(server->getStatusPage(404));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
		return;
	}

	// 2. Check with access if file is accessible
	if (access(filePath.c_str(), W_OK) != 0)
	{
		response.setStatus(403, "Forbidden");
		response.setBody(server->getStatusPage(403));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
		return;
	}

	// 3. If its a directory, check if it is empty
	if (S_ISDIR(fileStat.st_mode))
	{
		// Check if the directory is empty
		if (FILE_UTILS::isDirectoryEmpty(filePath))
		{
			// Delete the directory
			if (rmdir(filePath.c_str()) != 0)
			{
				response.setStatus(500, "Internal Server Error");
				response.setBody(server->getStatusPage(500));
				response.setHeader("Content-Type", "text/html");
				response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
				return;
			}
			else
			{
				response.setStatus(200, "OK");
				response.setBody(server->getStatusPage(200));
				response.setHeader("Content-Type", "text/html");
				response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
				return;
			}
		}
		else
		{
			response.setStatus(403, "Forbidden");
			response.setBody(server->getStatusPage(403));
			response.setHeader("Content-Type", "text/html");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
			return;
		}
	}
	else if (S_ISREG(fileStat.st_mode))
	{
		// Delete the file
		if (unlink(filePath.c_str()) != 0)
		{
			response.setStatus(500, "Internal Server Error");
			response.setBody(server->getStatusPage(500));
			response.setHeader("Content-Type", "text/html");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
			return;
		}
		else
		{
			response.setStatus(200, "OK");
			response.setBody(server->getStatusPage(200));
			response.setHeader("Content-Type", "text/html");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
			return;
		}
	}
	else
	{
		response.setStatus(501, "Not Implemented");
		response.setBody(server->getStatusPage(501));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
		return;
	}
}

} // namespace DELETE

#endif /* METHODHANDLERS_HPP */