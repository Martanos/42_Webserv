#include "../../includes/Core/GetMethodHandler.hpp"
#include "../../includes/Global/MimeTypeResolver.hpp"
#include <dirent.h>
#include <iomanip>

GetMethodHandler::GetMethodHandler()
{
}

GetMethodHandler::GetMethodHandler(const GetMethodHandler &other)
{
	*this = other;
}

GetMethodHandler::~GetMethodHandler()
{
}

GetMethodHandler &GetMethodHandler::operator=(const GetMethodHandler &other)
{
	(void)other;
	return *this;
}

bool GetMethodHandler::handleRequest(const HttpRequest &request, HttpResponse &response, 
									const Server *server, const Location *location)
{
	if (!canHandle(request.getMethod()))
	{
		response.setStatus(405, "Method Not Allowed");
		return false;
	}

	std::string uri = request.getUri();
	std::string rootPath = location->hasRoot() ? location->getRoot() : server->getRootPath();
	std::string filePath = rootPath + uri;

	Logger::debug("GetMethodHandler: Serving file: " + filePath);

	// Check if it's a directory
	if (isDirectory(filePath))
	{
		return serveDirectory(filePath, response, server, location);
	}
	else if (fileExists(filePath))
	{
		return serveFile(filePath, response, server, location);
	}
	else
	{
		// Check for index files
		if (location->hasIndexes())
		{
			const std::vector<std::string> &indexes = location->getIndexes().getAllValues();
			for (std::vector<std::string>::const_iterator it = indexes.begin(); it != indexes.end(); ++it)
			{
				std::string indexPath = filePath + "/" + *it;
				if (fileExists(indexPath))
				{
					return serveFile(indexPath, response, server, location);
				}
			}
		}

		// Check server-level index files
		if (server->hasIndex("index.html"))
		{
			const std::vector<std::string> &indexes = server->getIndexes().getAllValues();
			for (std::vector<std::string>::const_iterator it = indexes.begin(); it != indexes.end(); ++it)
			{
				std::string indexPath = filePath + "/" + *it;
				if (fileExists(indexPath))
				{
					return serveFile(indexPath, response, server, location);
				}
			}
		}

		// File not found
		response.setStatus(404, "Not Found");
		return false;
	}
}

bool GetMethodHandler::canHandle(const std::string &method) const
{
	return method == "GET";
}

bool GetMethodHandler::serveFile(const std::string &filePath, HttpResponse &response, 
								const Server *server, const Location *location)
{
	(void)server;
	(void)location;
	std::ifstream file(filePath.c_str(), std::ios::binary);
	if (!file.is_open())
	{
		response.setStatus(403, "Forbidden");
		return false;
	}

	// Read file content
	std::ostringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();
	file.close();

	// Set response
	response.setStatus(200, "OK");
	response.setBody(content);
	response.setHeader(Header("Content-Type: " + getMimeType(filePath)));
	response.setHeader(Header("Content-Length: " + StrUtils::toString(content.length())));

	Logger::debug("GetMethodHandler: Successfully served file: " + filePath);
	return true;
}

bool GetMethodHandler::serveDirectory(const std::string &dirPath, HttpResponse &response, 
									 const Server *server, const Location *location)
{
	// Check if autoindex is enabled
		if (location->hasAutoIndex())
	{
		std::string listing = generateDirectoryListing(dirPath, "");
		response.setStatus(200, "OK");
		response.setBody(listing);
		response.setHeader(Header("Content-Type: text/html"));
		response.setHeader(Header("Content-Length: " + StrUtils::toString(listing.length())));
		return true;
	}
		else if (server->isAutoIndex())
	{
		std::string listing = generateDirectoryListing(dirPath, "");
		response.setStatus(200, "OK");
		response.setBody(listing);
		response.setHeader(Header("Content-Type: text/html"));
		response.setHeader(Header("Content-Length: " + StrUtils::toString(listing.length())));
		return true;
	}
	else
	{
		response.setStatus(403, "Forbidden");
		return false;
	}
}

std::string GetMethodHandler::generateDirectoryListing(const std::string &dirPath, const std::string &uri)
{
	std::ostringstream html;
	html << "<!DOCTYPE html>\n";
	html << "<html><head><title>Index of " << uri << "</title></head>\n";
	html << "<body><h1>Index of " << uri << "</h1><hr><pre>\n";

	DIR *dir = opendir(dirPath.c_str());
	if (dir)
	{
		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL)
		{
			std::string name = entry->d_name;
			if (name == "." || name == "..")
				continue;

			std::string fullPath = dirPath + "/" + name;
			struct stat st;
			if (stat(fullPath.c_str(), &st) == 0)
			{
				if (S_ISDIR(st.st_mode))
					name += "/";
				
				html << "<a href=\"" << name << "\">" << name << "</a>\n";
			}
		}
		closedir(dir);
	}

	html << "</pre><hr></body></html>\n";
	return html.str();
}

std::string GetMethodHandler::getMimeType(const std::string &filePath)
{
	// Simple MIME type detection based on file extension
	size_t dotPos = filePath.find_last_of('.');
	if (dotPos == std::string::npos)
		return "application/octet-stream";

	std::string extension = filePath.substr(dotPos + 1);
	StrUtils::toLowerCase(extension);

	if (extension == "html" || extension == "htm")
		return "text/html";
	else if (extension == "css")
		return "text/css";
	else if (extension == "js")
		return "application/javascript";
	else if (extension == "json")
		return "application/json";
	else if (extension == "png")
		return "image/png";
	else if (extension == "jpg" || extension == "jpeg")
		return "image/jpeg";
	else if (extension == "gif")
		return "image/gif";
	else if (extension == "txt")
		return "text/plain";
	else
		return "application/octet-stream";
}

bool GetMethodHandler::isDirectory(const std::string &path)
{
	struct stat st;
	return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

bool GetMethodHandler::fileExists(const std::string &path)
{
	struct stat st;
	return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
}