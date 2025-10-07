#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include "FileDescriptor.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Location.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "StringUtils.hpp"
#include <cstddef>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// System constants all in one place
// HTTP Constants
namespace HTTP
{
const char *const SINGLETON_HEADERS[] = {
	"host",			 "content-length",		"content-type", "content-location", "date",		   "etag",
	"expires",		 "last-modified",		"location",		"server",			"user-agent",  "referer",
	"authorization", "proxy-authorization", "expect",		"upgrade",			"retry-after", "content-range"};
static const std::string SUPPORTED_METHODS[] = {"GET", "POST", "DELETE"};
static const std::string HTTP_VERSION = "HTTP/1.1";
static const std::string TEMP_FILE_TEMPLATE = "/tmp/webserv-";
static const ssize_t MAX_URI_LINE_SIZE = 16384;		 // 16KB
static const ssize_t MAX_HEADERS_LINE_SIZE = 16384;	 // 16KB
static const ssize_t MAX_HEADERS_SIZE = 32768;		 // 32KB
static const ssize_t MAX_BODY_BUFFER_SIZE = 1048576; // 1MB
static const ssize_t MAX_BODY_SIZE = 10485760;		 // 10MB
static const char *const CRLF = "\r\n";				 // CRLF
const int DEFAULT_TIMEOUT_SECONDS = 30;				 // 30 second timeout

} // namespace HTTP

namespace HTTP_PARSING_UTILS
{
std::string percentDecode(const std::string &input)
{
	std::string output;
	for (size_t i = 0; i < input.size(); ++i)
	{
		if (input[i] == '%' && i + 2 < input.size())
		{
			char hex[3] = {input[i + 1], input[i + 2], '\0'};
			if (isxdigit(hex[0]) && isxdigit(hex[1]))
			{
				char *end;
				output += static_cast<char>(strtol(hex, &end, 16));
				if (*end != '\0')
				{
					output += '%'; // Preserve malformed %
				}
				i += 2;
			}
			else
			{
				output += '%'; // Preserve malformed %
			}
		}
		else
		{
			output += input[i];
		}
	}
	return output;
}

} // namespace HTTP_PARSING_UTILS

// TODO: Move string utils here
namespace STRING_UTILS
{

} // namespace STRING_UTILS

// Server Constants
namespace SERVER
{
const char *const DEFAULT_HOST = "0.0.0.0";
const unsigned short DEFAULT_PORT = 80;
const char *const DEFAULT_ROOT = "www/";
const char *const DEFAULT_INDEX = "index.html";
const bool DEFAULT_AUTOINDEX = false;
const size_t DEFAULT_CLIENT_MAX_BODY_SIZE = 1048576; // 1MB
const bool DEFAULT_KEEP_ALIVE = true;

const char *const SERVER_VERSION = "42_Webserv/1.0";
} // namespace SERVER

namespace FILE_UTILS
{

// TODO: Move to FileDescriptor / FileManager
// Check if path is accessible
static bool isPathAccessible(const std::string &path)
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
	{
		return false;
	}

	// Check read permission
	if (access(path.c_str(), R_OK) != 0)
	{
		return false;
	}
	return true;
}

// TODO: Move to FileDescriptor / FileManager
// Read file using FileDescriptor
static std::string readFileWithFd(const std::string &filePath)
{
	FileDescriptor fd = FileDescriptor::createFromOpen(filePath.c_str(), O_RDONLY);

	if (!fd.isValid())
	{
		Logger::log(Logger::ERROR, "Cannot open file: " + filePath);
		throw std::runtime_error("Cannot open file");
	}

	// Get file size
	struct stat fileStat;
	if (fstat(fd.getFd(), &fileStat) != 0)
	{
		throw std::runtime_error("Cannot stat file");
	}

	// Read file content
	std::string content;
	content.resize(fileStat.st_size);

	ssize_t totalRead = 0;
	while (totalRead < fileStat.st_size)
	{
		ssize_t bytesRead = read(fd.getFd(), &content[totalRead], fileStat.st_size - totalRead);
		if (bytesRead < 0)
		{
			throw std::runtime_error("Error reading file");
		}
		else if (bytesRead == 0)
		{
			break; // End of file
		}
		totalRead += bytesRead;
	}

	content.resize(totalRead);
	return content;
}

// TODO: Move to FileDescriptor / FileManager
// Write file using FileDescriptor
static bool writeFileWithFd(const std::string &filePath, const std::string &content)
{
	FileDescriptor fd = FileDescriptor::createFromOpen(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
													   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (!fd.isValid())
	{
		Logger::log(Logger::ERROR, "Cannot create file: " + filePath);
		return false;
	}

	size_t totalWritten = 0;
	while (totalWritten < content.length())
	{
		ssize_t written = write(fd.getFd(), content.c_str() + totalWritten, content.length() - totalWritten);
		if (written < 0)
		{
			Logger::log(Logger::ERROR, "Error writing file: " + filePath);
			return false;
		}
		totalWritten += written;
	}

	return true;
}

} // namespace FILE_UTILS

namespace METHOD_UTILS
{

// Deletion method utilities
static void deleteMethodHandler(const HttpRequest &request, HttpResponse &response, const Server *server,
								const Location *location)
{
	std::string filePath = absolutePath(request.getUri());

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

} // namespace METHOD_UTILS

#endif /* CONSTANTS_HPP */
