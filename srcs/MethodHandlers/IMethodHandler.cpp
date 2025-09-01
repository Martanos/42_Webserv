#include "IMethodHandler.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

IMethodHandler::IMethodHandler()
{
}

IMethodHandler::IMethodHandler(const IMethodHandler &src)
{
	(void)src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

IMethodHandler::~IMethodHandler()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

IMethodHandler &IMethodHandler::operator=(IMethodHandler const &rhs)
{
	(void)rhs;
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

// Resolve file path based on URI and server/location configuration
std::string IMethodHandler::resolveFilePath(const std::string &uri,
											const Server *server,
											const Location *location)
{
	// Remove query string if present
	std::string cleanUri = uri;
	size_t queryPos = cleanUri.find('?');
	if (queryPos != std::string::npos)
	{
		cleanUri = cleanUri.substr(0, queryPos);
	}

	// Remove fragment if present
	size_t fragmentPos = cleanUri.find('#');
	if (fragmentPos != std::string::npos)
	{
		cleanUri = cleanUri.substr(0, fragmentPos);
	}

	// Get root directory
	std::string root;
	if (location && !location->getRoot().empty())
	{
		root = location->getRoot();
	}
	else if (server)
	{
		root = server->getRoot();
	}
	else
	{
		root = "www"; // Default fallback
	}

	// Ensure root doesn't end with slash
	if (!root.empty() && root[root.length() - 1] == '/')
	{
		root = root.substr(0, root.length() - 1);
	}

	// Ensure URI starts with slash
	if (cleanUri.empty() || cleanUri[0] != '/')
	{
		cleanUri = "/" + cleanUri;
	}

	// If location has a path, remove it from URI
	if (location && !location->getPath().empty())
	{
		std::string locPath = location->getPath();
		if (cleanUri.find(locPath) == 0)
		{
			cleanUri = cleanUri.substr(locPath.length());
			if (cleanUri.empty())
			{
				cleanUri = "/";
			}
			else if (cleanUri[0] != '/')
			{
				cleanUri = "/" + cleanUri;
			}
		}
	}

	return root + cleanUri;
}

// Get MIME type from file extension
std::string IMethodHandler::getMimeType(const std::string &filePath)
{
	// Use the MimeTypes class if available
	size_t dotPos = filePath.find_last_of('.');
	if (dotPos != std::string::npos && dotPos < filePath.length() - 1)
	{
		std::string extension = filePath.substr(dotPos + 1);

		// Try to get from MimeTypes class
		std::string mimeType = MimeTypes::getMimeType(extension);
		if (!mimeType.empty())
		{
			return mimeType;
		}
	}

	// Default fallback for common types
	size_t lastDot = filePath.find_last_of('.');
	if (lastDot != std::string::npos)
	{
		std::string ext = filePath.substr(lastDot + 1);

		// Convert to lowercase for comparison
		for (size_t i = 0; i < ext.length(); ++i)
		{
			ext[i] = std::tolower(ext[i]);
		}

		// Common MIME types
		if (ext == "html" || ext == "htm")
			return "text/html";
		if (ext == "css")
			return "text/css";
		if (ext == "js")
			return "application/javascript";
		if (ext == "json")
			return "application/json";
		if (ext == "xml")
			return "application/xml";
		if (ext == "txt")
			return "text/plain";
		if (ext == "jpg" || ext == "jpeg")
			return "image/jpeg";
		if (ext == "png")
			return "image/png";
		if (ext == "gif")
			return "image/gif";
		if (ext == "svg")
			return "image/svg+xml";
		if (ext == "ico")
			return "image/x-icon";
		if (ext == "pdf")
			return "application/pdf";
		if (ext == "zip")
			return "application/zip";
		if (ext == "mp3")
			return "audio/mpeg";
		if (ext == "mp4")
			return "video/mp4";
		if (ext == "webm")
			return "video/webm";
	}

	// Default binary type
	return "application/octet-stream";
}

// Check if path is accessible
bool IMethodHandler::isPathAccessible(const std::string &path)
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

// TODO: Integrate this into FileDescriptor class
// Read file using FileDescriptor
std::string IMethodHandler::readFileWithFd(const std::string &filePath)
{
	FileDescriptor fd(open(filePath.c_str(), O_RDONLY));

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
		ssize_t bytesRead = read(fd.getFd(),
								 &content[totalRead],
								 fileStat.st_size - totalRead);
		if (bytesRead < 0)
		{
			if (errno == EINTR)
			{
				continue; // Interrupted, try again
			}
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

// TODO: Integrate this into FileDescriptor class
// Write file using FileDescriptor
bool IMethodHandler::writeFileWithFd(const std::string &filePath,
									 const std::string &content)
{
	FileDescriptor fd(open(filePath.c_str(),
						   O_WRONLY | O_CREAT | O_TRUNC,
						   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));

	if (!fd.isValid())
	{
		Logger::log(Logger::ERROR, "Cannot create file: " + filePath);
		return false;
	}

	size_t totalWritten = 0;
	while (totalWritten < content.length())
	{
		ssize_t written = write(fd.getFd(),
								content.c_str() + totalWritten,
								content.length() - totalWritten);
		if (written < 0)
		{
			if (errno == EINTR)
			{
				continue; // Interrupted, try again
			}
			Logger::log(Logger::ERROR, "Error writing file: " + filePath);
			return false;
		}
		totalWritten += written;
	}

	return true;
}

// Set common response headers
void IMethodHandler::setCommonHeaders(HttpResponse &response,
									  const std::string &contentType,
									  size_t contentLength)
{
	response.setHeader("Content-Type", contentType);
	response.setHeader("Content-Length", StringUtils::toString(contentLength));
	response.setHeader("Server", "42_Webserv/1.0");

	// Add current date
	time_t now = time(NULL);
	char dateBuffer[100];
	struct tm *tm = gmtime(&now);
	strftime(dateBuffer, sizeof(dateBuffer),
			 "%a, %d %b %Y %H:%M:%S GMT", tm);
	response.setHeader("Date", std::string(dateBuffer));
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

/* ************************************************************************** */
