#include "GetMethodHandler.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

GetMethodHandler::GetMethodHandler()
{
}

GetMethodHandler::GetMethodHandler(const GetMethodHandler &src)
{
	(void)src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

GetMethodHandler::~GetMethodHandler()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

GetMethodHandler &GetMethodHandler::operator=(GetMethodHandler const &rhs)
{
	(void)rhs;
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

// Main handler method
void GetMethodHandler::handle(const HttpRequest &request, HttpResponse &response, const Server *server,
							  const Location *location)
{
	try
	{
		// Resolve the file path based on URI and configuration
		std::string filePath = resolveFilePath(request.getUri(), server, location);

		// Check if there's a redirect configured
		if (location && !location->getRedirect().empty())
		{
			response.setStatus(301, "Moved Permanently");
			response.setHeader("Location", location->getRedirect());
			response.setHeader("Content-Length", "0");
			return;
		}

		// Get file statistics
		struct stat fileStat;
		if (stat(filePath.c_str(), &fileStat) != 0)
		{
			Logger::log(Logger::WARNING, "File not found: " + filePath);
			response.setStatus(404, "Not Found");
			response.setBody(server->getStatusPage(404));
			response.setHeader("Content-Type", "text/html");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
			return;
		}

		// Check if it's a directory
		if (S_ISDIR(fileStat.st_mode))
		{
			// Ensure trailing slash for directories
			std::string uri = request.getUri();
			if (!uri.empty() && uri[uri.length() - 1] != '/')
			{
				response.setStatus(301, "Moved Permanently");
				response.setHeader("Location", uri + "/");
				response.setHeader("Content-Length", "0");
				return;
			}

			// Get index files from location or server config
			std::vector<std::string> indexes;
			if (location && !location->getIndex().empty())
			{
				indexes.push_back(location->getIndex());
			}
			else
			{
				indexes = server->getIndexes();
			}

			// Try to serve index files
			if (!indexes.empty() && tryIndexFiles(filePath, indexes, response))
			{
				return;
			}

			// Check if autoindex is enabled
			bool autoindex = location ? location->getAutoIndex() : server->getAutoindex();

			if (autoindex)
			{
				generateDirectoryListing(filePath, request.getUri(), response, server, location);
			}
			else
			{
				response.setStatus(403, "Forbidden");
				response.setBody(server->getStatusPage(403));
				response.setHeader("Content-Type", "text/html");
				response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
			}
		}
		else if (S_ISREG(fileStat.st_mode))
		{
			// Serve regular file
			serveFile(filePath, response);
		}
		else
		{
			// Not a regular file or directory
			response.setStatus(403, "Forbidden");
			response.setBody(server->getStatusPage(403));
			response.setHeader("Content-Type", "text/html");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
		}

		// Handle HEAD requests (no body)
		if (request.getMethod() == "HEAD")
		{
			response.setBody("");
		}
	}
	catch (const std::exception &e)
	{
		Logger::log(Logger::ERROR, "Error handling GET request: " + std::string(e.what()));
		response.setStatus(500, "Internal Server Error");
		response.setBody(server->getStatusPage(500));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
	}
}

// Serve a regular file using FileDescriptor for safe memory management
void GetMethodHandler::serveFile(const std::string &filePath, HttpResponse &response) const
{
	// Use FileDescriptor for RAII file management
	FileDescriptor fd(open(filePath.c_str(), O_RDONLY));

	if (!fd.isValid())
	{
		Logger::log(Logger::ERROR, "Cannot open file: " + filePath);
		response.setStatus(403, "Forbidden");
		return;
	}

	// Get file size
	struct stat fileStat;
	if (fstat(fd.getFd(), &fileStat) != 0)
	{
		Logger::log(Logger::ERROR, "Cannot stat file: " + filePath);
		response.setStatus(500, "Internal Server Error");
		return;
	}

	// Check file size (prevent loading huge files into memory)
	const size_t MAX_FILE_SIZE = 100 * 1024 * 1024; // 100MB limit
	if (fileStat.st_size > MAX_FILE_SIZE)
	{
		Logger::log(Logger::ERROR, "File too large: " + filePath);
		response.setStatus(413, "Payload Too Large");
		return;
	}

	// Read file content using FileDescriptor's readFile method
	std::string content;
	content.resize(fileStat.st_size);
	ssize_t bytesRead = read(fd.getFd(), &content[0], fileStat.st_size);

	if (bytesRead < 0 || bytesRead != fileStat.st_size)
	{
		Logger::log(Logger::ERROR, "Error reading file: " + filePath);
		response.setStatus(500, "Internal Server Error");
		return;
	}

	// Set response
	response.setStatus(200, "OK");
	response.setBody(content);
	response.setHeader("Content-Type", MimeTypes::getMimeType(filePath));
	response.setHeader("Content-Length", StringUtils::toString(content.length()));

	// Add Last-Modified header
	char buffer[100];
	struct tm *tm = gmtime(&fileStat.st_mtime);
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", tm);
	response.setHeader("Last-Modified", std::string(buffer));

	// FileDescriptor destructor will automatically close the file
}

// Try to serve index files from a directory
bool GetMethodHandler::tryIndexFiles(const std::string &dirPath, const std::vector<std::string> &indexes,
									 HttpResponse &response) const
{
	for (std::vector<std::string>::const_iterator it = indexes.begin(); it != indexes.end(); ++it)
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

// Generate directory listing HTML
void GetMethodHandler::generateDirectoryListing(const std::string &dirPath, const std::string &uri,
												HttpResponse &response, const Server *server,
												const Location *location) const
{
	(void)server;
	(void)location;

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
		if (!isHiddenFile(name))
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
			html << "<td><a href=\"" << urlEncode(*it);
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
				html << formatFileSize(entryStat.st_size);
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
	response.setStatus(200, "OK");
	response.setBody(htmlContent);
	response.setHeader("Content-Type", "text/html");
	response.setHeader("Content-Length", StringUtils::toString(htmlContent.length()));
}

// Format file size for display
std::string GetMethodHandler::formatFileSize(off_t size) const
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

// URL encode a string
std::string GetMethodHandler::urlEncode(const std::string &str) const
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

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

// Check if file should be hidden
bool GetMethodHandler::isHiddenFile(const std::string &filename) const
{
	return filename.empty() || filename[0] == '.' || filename == "." || filename == "..";
}

bool GetMethodHandler::canHandle(const std::string &method) const
{
	return method == "GET" || method == "HEAD";
}

std::string GetMethodHandler::getMethod() const
{
	return "GET";
}

/* ************************************************************************** */
