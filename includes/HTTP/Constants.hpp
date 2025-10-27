#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include "../../includes/Global/Logger.hpp"
#include "../../includes/Wrapper/FileDescriptor.hpp"
#include <cstddef>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// System constants all in one place
// HTTP Constants
namespace HTTP
{
const char *const SINGLETON_HEADERS[] = {
	"host",			 "content-length",		"content-type", "content-location", "date",		   "etag",
	"expires",		 "last-modified",		"location",		"server",			"user-agent",  "referer",
	"authorization", "proxy-authorization", "expect",		"upgrade",			"retry-after", "content-range"};
static const std::vector<std::string> SUPPORTED_METHODS = {"GET", "POST", "DELETE"};
static const std::string HTTP_VERSION = "HTTP/1.1";
static const std::string TEMP_FILE_TEMPLATE = "/tmp/webserv-";
static const ssize_t MAX_URI_LINE_SIZE = 16384;		 // 16KB
static const ssize_t MAX_HEADERS_LINE_SIZE = 16384;	 // 16KB
static const ssize_t MAX_HEADERS_SIZE = 32768;		 // 32KB
static const ssize_t MAX_BODY_BUFFER_SIZE = 1048576; // 1MB
static const ssize_t MAX_BODY_SIZE = 10485760;		 // 10MB
static const char *const CRLF = "\r\n";				 // CRLF
const int DEFAULT_TIMEOUT_SECONDS = 30;				 // 30 second timeout
static const std::string DEFAULT_HOST = "0.0.0.0";
static const unsigned short DEFAULT_PORT = 80;
static const bool DEFAULT_AUTOINDEX = false;
static const bool DEFAULT_KEEP_ALIVE = true;

bool isSupportedMethod(const std::string &method)
{
	for (size_t i = 0; i < sizeof(HTTP::SUPPORTED_METHODS) / sizeof(HTTP::SUPPORTED_METHODS[0]); ++i)
	{
		if (SUPPORTED_METHODS[i] == method)
			return true;
	}
	return false;
}

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

static bool isDirectoryEmpty(const std::string &path)
{
	DIR *dir = opendir(path.c_str());
	if (!dir)
	{
		// Could not open directory — maybe doesn't exist or permission denied
		return false;
	}

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		// Skip "." and ".."
		if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
		{
			closedir(dir);
			return false; // Found something
		}
	}

	closedir(dir);
	return true; // Only "." and ".." found
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

#endif /* CONSTANTS_HPP */
