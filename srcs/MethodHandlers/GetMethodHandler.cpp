#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/MimeTypeResolver.hpp"
#include "../../includes/MethodHandlers/GetMethodHandler.hpp"
#include "../../includes/Utils/FileUtils.hpp"
#include <dirent.h>
#include <fstream>
#include <sstream>

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
	return (*this);
}

bool GetMethodHandler::handleRequest(const HttpRequest &request,
	HttpResponse &response, const Location &location)
{
	// Get sanitized file path
	std::string filePath = request.getSanitizedUri();
	Logger::debug("GetMethodHandler: Serving file: " + filePath, __FILE__,
		__LINE__, __PRETTY_FUNCTION__);
	// Directory path handling
	if (FileUtils::isDirectory(filePath))
	{
		if (location.hasIndexDirective())
		{
			const std::vector<std::string> &indexes = location.getIndexes()->getAllValues();
			for (std::vector<std::string>::const_iterator it = indexes.begin(); it != indexes.end(); ++it)
			{
				std::string indexPath = filePath + "/" + *it;
				Logger::debug("GetMethodHandler: Checking location index: "
					+ indexPath, __FILE__, __LINE__, __PRETTY_FUNCTION__);
				if (FileUtils::fileExists(indexPath))
				{
					Logger::debug("GetMethodHandler: Serving location index file: "
						+ indexPath, __FILE__, __LINE__, __PRETTY_FUNCTION__);
					return (serveFile(indexPath, response, location));
				}
			}
		}
		if (location.hasAutoIndexDirective())
		{
			if (location.getAutoIndexValue())
			{
				std::string listing = generateDirectoryListing(filePath);
				response.setResponseCustomBody(200, "OK", listing, "text/html",
					HttpResponse::SUCCESS);
				return (true);
			}
		}
		response.setResponseDefaultBody(403, "Forbidden", &location,
			HttpResponse::ERROR);
		return (false);
	}
	else if (FileUtils::fileExists(filePath))
	{
		return (serveFile(filePath, response, location));
	}
	// File not found
	response.setResponseDefaultBody(404, "Not Found", &location,
		HttpResponse::ERROR);
	return (false);
}

bool GetMethodHandler::serveFile(const std::string &filePath,
	HttpResponse &response, const Location &location)
{
	std::ifstream file(filePath.c_str(), std::ios::binary);
	if (!file.is_open())
	{
		response.setResponseDefaultBody(403, "Cannot access file: " + filePath,
			&location, HttpResponse::ERROR);
		return (false);
	}
	// Set response
	response.setResponseFile(200, "OK", filePath,
		MimeTypeResolver::resolveMimeType(filePath), HttpResponse::SUCCESS);
	Logger::debug("GetMethodHandler: Successfully served file: " + filePath,
		__FILE__, __LINE__, __PRETTY_FUNCTION__);
	return (true);
}

std::string GetMethodHandler::generateDirectoryListing(const std::string &filePath)
{
	DIR				*dir;
	struct dirent	*entry;
	struct stat		st;

	std::ostringstream html;
	html << "<!DOCTYPE html>\n";
	html << "<html><head><title>Index of " << filePath << "</title></head>\n";
	html << "<body><h1>Index of " << filePath << "</h1><hr><pre>\n";
	dir = opendir(filePath.c_str());
	if (dir)
	{
		while ((entry = readdir(dir)) != NULL)
		{
			std::string name = entry->d_name;
			if (name == "." || name == "..")
				continue ;
			std::string fullPath = filePath + "/" + name;
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
	return (html.str());
}
