#ifndef GETHANDLER_HPP
#define GETHANDLER_HPP

#include "../../includes/Core/Location.hpp"
#include "../../includes/Core/Server.hpp"
#include "../../includes/Global/DefaultStatusMap.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/MimeTypeResolver.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/HTTP/HttpRequest.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
#include "../../includes/Wrapper/FileDescriptor.hpp"
#include <dirent.h>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <vector>

namespace GET
{
// Nested namespace for GET utilities
namespace GET_UTILS
{

static inline bool isHiddenFile(const std::string &filename)
{
	return filename.empty() || filename[0] == '.' || filename == "." || filename == "..";
}

// TODO: Move to response class/FileManager serveFile should just read the file into the kernel buffer no need to laod
// anything into memory
static inline void serveFile(const std::string &filePath, HttpResponse &response)
{
	// Use FileDescriptor for RAII file management
	FileDescriptor fd = FileDescriptor::createFromOpen(filePath.c_str(), O_RDONLY);

	if (!fd.isValid())
		return response.setResponse(500, "Cannot access file: " + filePath, NULL, NULL, filePath);

	// Get file size
	struct stat fileStat;
	if (fstat(fd.getFd(), &fileStat) != 0)
		return response.setResponse(500, "Cannot get file statistics", NULL, NULL, filePath);

	// Check file size (prevent loading huge files into memory)
	const size_t MAX_FILE_SIZE = 100 * 1024 * 1024; // 100MB limit
	if (static_cast<size_t>(fileStat.st_size) > MAX_FILE_SIZE)
		return response.setResponse(413, "Payload Too Large", NULL, NULL, filePath);

	// Read file content using FileDescriptor's readFile method
	response.setBody(fd.readFile());

	// Set response
	response.setResponse(200, "OK", NULL, NULL, filePath);

	// Add Last-Modified header
	response.setLastModifiedHeader();
}

// Try to serve index files from a directory
static inline bool tryIndexFiles(const std::string &dirPath, const TrieTree<std::string> &indexes,
								 HttpResponse &response)
{
	for (TrieTree<std::string>::const_iterator it = indexes.begin(); it != indexes.end(); ++it)
	{
		std::string indexPath = dirPath;
		if (indexPath[indexPath.length() - 1] != '/')
		{
			indexPath += '/';
		}
		indexPath += *it;

		struct stat fileStat;
		if (stat(indexPath.c_str(), &fileStat) == 0 && S_ISREG(fileStat.st_mode))
		{
			serveFile(indexPath, response);
			return true;
		}
	}
	return false;
}

// URL encode a string
// TODO: Move to StrUtils
static inline std::string urlEncode(const std::string &str)
{
	std::stringstream encoded;
	for (size_t i = 0; i < str.length(); ++i)
	{
		unsigned char c = str[i];
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
			c == '.' || c == '~')
		{
			encoded << c;
		}
		else
		{
			encoded << '%' << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(c);
		}
	}
	return encoded.str();
}

// Format file size for display TODO: Move to StrUtils
static inline std::string formatFileSize(off_t size)
{
	std::stringstream ss;
	if (size < 1024)
	{
		ss << size << " B";
	}
	else if (size < 1024 * 1024)
	{
		ss << std::fixed << std::setprecision(1) << (size / 1024.0) << " KB";
	}
	else if (size < 1024 * 1024 * 1024)
	{
		ss << std::fixed << std::setprecision(1) << (size / (1024.0 * 1024.0)) << " MB";
	}
	else
	{
		ss << std::fixed << std::setprecision(1) << (size / (1024.0 * 1024.0 * 1024.0)) << " GB";
	}
	return ss.str();
}

// Generate directory listing HTML
static inline void generateDirectoryListing(const std::string &dirPath, const std::string &uri, HttpResponse &response)
{
	DIR *dir = opendir(dirPath.c_str());
	if (!dir)
	{
		response.setStatus(403, "Forbidden");
		return;
	}

	std::stringstream html;
	html << "<!DOCTYPE html>\n";
	html << "<html>\n<head>\n";
	html << "<title>Index of " << uri << "</title>\n";
	html << "<style>\n";
	html << "body { font-family: monospace; margin: 20px; }\n";
	html << "h1 { font-size: 24px; }\n";
	html << "table { border-collapse: collapse; }\n";
	html << "th, td { padding: 5px 15px; text-align: left; }\n";
	html << "th { border-bottom: 1px solid #000; }\n";
	html << "a { text-decoration: none; color: #0066cc; }\n";
	html << "a:hover { text-decoration: underline; }\n";
	html << ".size { text-align: right; }\n";
	html << "</style>\n";
	html << "</head>\n<body>\n";
	html << "<h1>Index of " << uri << "</h1>\n";
	html << "<table>\n";
	html << "<tr><th>Name</th><th>Last Modified</th><th "
			"class=\"size\">Size</th></tr>\n";

	// Add parent directory link if not root
	if (uri != "/")
	{
		html << "<tr><td colspan=\"3\"><a href=\"../\">../</a></td></tr>\n";
	}

	// Read directory entries
	std::vector<std::string> entries;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		if (!GET_UTILS::isHiddenFile(name))
		{
			entries.push_back(name);
		}
	}
	closedir(dir);

	// Sort entries (directories first, then alphabetically)
	std::sort(entries.begin(), entries.end());

	// Generate table rows
	for (std::vector<std::string>::const_iterator it = entries.begin(); it != entries.end(); ++it)
	{
		std::string entryPath = dirPath + "/" + *it;
		struct stat entryStat;

		if (stat(entryPath.c_str(), &entryStat) == 0)
		{
			html << "<tr>";

			// Name column
			html << "<td><a href=\"" << GET_UTILS::urlEncode(*it);
			if (S_ISDIR(entryStat.st_mode))
			{
				html << "/";
			}
			html << "\">" << *it;
			if (S_ISDIR(entryStat.st_mode))
			{
				html << "/";
			}
			html << "</a></td>";

			// Last Modified column
			char timeBuffer[80];
			struct tm *tm = gmtime(&entryStat.st_mtime);
			strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", tm);
			html << "<td>" << timeBuffer << "</td>";

			// Size column
			html << "<td class=\"size\">";
			if (S_ISDIR(entryStat.st_mode))
			{
				html << "-";
			}
			else
			{
				html << GET_UTILS::formatFileSize(entryStat.st_size);
			}
			html << "</td>";

			html << "</tr>\n";
		}
	}

	html << "</table>\n";
	html << "<hr>\n";
	html << "<address>42_Webserv/1.0</address>\n";
	html << "</body>\n</html>\n";

	std::string htmlContent = html.str();
	response.setResponse(200, "OK", NULL, NULL, dirPath);
	response.setBody(htmlContent);
}

} // namespace GET_UTILS

static inline void handler(const HttpRequest &request, HttpResponse &response, const Server *server,
						   const Location *location)
{
	try
	{
		std::string filePath = request.getUri();
		if (filePath.empty())
			return response.setResponse(404, "Not Found", NULL, NULL, filePath);
		// Check if there's a redirect configured
		if (location && location->hasRedirect())
			return response.setResponse(301, "Moved Permanently", NULL, NULL, location->getRedirect().second);
		// Get file statistics
		struct stat fileStat;
		if (stat(filePath.c_str(), &fileStat) != 0)
			return response.setResponse(404, "Not Found", NULL, NULL, filePath);
		// Check if it's a directory
		if (S_ISDIR(fileStat.st_mode))
		{
			// Ensure trailing slash for directories
			std::string uri = request.getUri();
			if (!uri.empty() && uri[uri.length() - 1] != '/')
				return response.setResponse(301, "Moved Permanently", NULL, NULL, uri + "/");

			// If index files are located in the location, try to serve them
			if (location && location->hasIndexes())
				if (GET_UTILS::tryIndexFiles(filePath, location->getIndexes(), response))
					return;

			// Check if autoindex is enabled
			bool autoindex = location->hasAutoIndex();

			// If autoindex is enabled, generate directory listing
			if (autoindex)
				return GET_UTILS::generateDirectoryListing(filePath, request.getUri(), response);
			else
				return response.setResponse(403, "Forbidden", NULL, NULL, filePath);
		}
		else if (S_ISREG(fileStat.st_mode))
		{
			// If it's a regular file, serve it
			return GET_UTILS::serveFile(filePath, response);
		}
		// Not a regular file or directory
		return response.setResponse(403, "Forbidden", NULL, NULL, filePath);

		// Handle HEAD requests (no body)
		if (request.getMethod() == "HEAD")
		{
			response.setBody("");
		}
	}
	catch (const std::exception &e)
	{
		Logger::log(Logger::ERROR, "Error handling GET request: " + std::string(e.what()));
		return response.setResponse(500, "Internal Server Error", NULL, NULL, request.getUri());
	}
}
} // namespace GET

#endif /* GETHANDLER_HPP */