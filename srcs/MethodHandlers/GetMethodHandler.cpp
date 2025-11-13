#include "../../includes/Core/GetMethodHandler.hpp"
#include "../../includes/Global/MimeTypeResolver.hpp"
#include <dirent.h>

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

bool GetMethodHandler::handleRequest(const HttpRequest &request, HttpResponse &response, const Server *server,
									 const Location *location)
{
	// Step 1: Validate method
	if (!canHandle(request.getMethod()))
		return false;

	// step 2: Get sanitized file path
	std::string filePath = request.getUri();

	Logger::debug("GetMethodHandler: Serving file: " + filePath, __FILE__, __LINE__, __PRETTY_FUNCTION__);

	// Check if it's a directory
	if (isDirectory(filePath))
	{
		// Location takes precedence over server for index and autoindex
		if (location)
		{
			if (location->hasIndexesDirective())
			{
				const std::vector<std::string> &indexes = location->getIndexes().getAllValues();
				for (std::vector<std::string>::const_iterator it = indexes.begin(); it != indexes.end(); ++it)
				{
					std::string indexPath = filePath + "/" + *it;
					Logger::debug("GetMethodHandler: Checking location index: " + indexPath, __FILE__, __LINE__,
								  __PRETTY_FUNCTION__);
					if (fileExists(indexPath))
					{
						Logger::debug("GetMethodHandler: Serving location index file: " + indexPath, __FILE__, __LINE__,
									  __PRETTY_FUNCTION__);
						return serveFile(indexPath, response, server, location);
					}
				}
			}
		}
		else if (server)
		{
			const std::vector<std::string> serverIndexes = server->getIndexes().getAllValues();
			for (std::vector<std::string>::const_iterator it = serverIndexes.begin(); it != serverIndexes.end(); ++it)
			{
				std::string indexPath = filePath + "/" + *it;
				Logger::debug("GetMethodHandler: Checking server index: " + indexPath, __FILE__, __LINE__,
							  __PRETTY_FUNCTION__);
				if (fileExists(indexPath))
				{
					Logger::debug("GetMethodHandler: Serving server index file: " + indexPath, __FILE__, __LINE__,
								  __PRETTY_FUNCTION__);
					return serveFile(indexPath, response, server, location);
				}
			}
		}
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
		response.setResponseDefaultBody(404, "Not Found", server, location, HttpResponse::ERROR);
		return false;
	}
}

bool GetMethodHandler::canHandle(const std::string &method) const
{
	return method == "GET";
}

bool GetMethodHandler::serveFile(const std::string &filePath, HttpResponse &response, const Server *server,
								 const Location *location)
{
	std::ifstream file(filePath.c_str(), std::ios::binary);
	if (!file.is_open())
	{
		response.setResponseDefaultBody(403, "Cannot access file: " + filePath, server, location, HttpResponse::ERROR);
		return false;
	}

	// Set response
	response.setResponseFile(200, "OK", filePath, MimeTypeResolver::resolveMimeType(filePath), HttpResponse::SUCCESS);

	Logger::debug("GetMethodHandler: Successfully served file: " + filePath, __FILE__, __LINE__, __PRETTY_FUNCTION__);
	return true;
}

bool GetMethodHandler::serveDirectory(const std::string &dirPath, HttpResponse &response, const Server *server,
									  const Location *location)
{
	if (location->hasAutoIndex())
	{
		if (location->isAutoIndex())
		{
			std::string listing = generateDirectoryListing(dirPath, "");
			response.setResponseCustomBody(200, "OK", listing, "text/html", HttpResponse::SUCCESS);
			return true;
		}
		else
		{
			response.setResponseDefaultBody(403, "Forbidden", server, location, HttpResponse::ERROR);
			return false;
		}
	}
	else if (server->hasAutoIndex())
	{
		if (server->isAutoIndex())
		{
			std::string listing = generateDirectoryListing(dirPath, "");
			response.setResponseCustomBody(200, "OK", listing, "text/html", HttpResponse::SUCCESS);
			return true;
		}
		else
		{
			response.setResponseDefaultBody(403, "Forbidden", server, location, HttpResponse::ERROR);
			return false;
		}
	}
	response.setResponseDefaultBody(403, "Forbidden", server, location, HttpResponse::ERROR);
	return false;
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
