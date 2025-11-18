#include "../../includes/MethodHandlers/GetMethodHandler.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/MimeTypeResolver.hpp"
#include "../../includes/Utils/FileUtils.hpp"
#include <dirent.h>
#include <string>

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

/*
** --------------------------------- HELPER METHODS ----------------------------------
*/

std::string GetMethodHandler::_generateDirectoryListing(const std::string &filePath)
{
	DIR *dir;
	struct dirent *entry;
	struct stat st;

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
				continue;
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

bool GetMethodHandler::_serveFile(const std::string &filePath, HttpResponse &response, const Location &location)
{
	if (!FileUtils::isFileReadable(filePath))
	{
		switch (errno)
		{
		case EACCES:
			response.setResponseDefaultBody(403, "Forbidden: File is not readable", &location, HttpResponse::ERROR);
			break;
		case ENOENT:
			response.setResponseDefaultBody(404, "Not Found: Target does not exist", &location, HttpResponse::ERROR);
			break;
		case ENOTDIR:
			response.setResponseDefaultBody(404, "Not Found: Invalid path component", &location, HttpResponse::ERROR);
			break;
		case ELOOP:
			response.setResponseDefaultBody(403, "Forbidden: Too many symlinks", &location, HttpResponse::ERROR);
			break;
		case EROFS:
			response.setResponseDefaultBody(403, "Forbidden: Read-only filesystem", &location, HttpResponse::ERROR);
			break;
		case ETXTBSY:
			response.setResponseDefaultBody(409, "Conflict: File is busy", &location, HttpResponse::ERROR);
			break;
		default:
			response.setResponseDefaultBody(500, "Internal Server Error: Unable to read file", &location,
											HttpResponse::ERROR);
			break;
		}
		Logger::error("GetMethodHandler: Cannot read file: " + filePath + " - " + std::string(strerror(errno)),
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
		return (false);
	}
	// Set response
	response.setResponseFile(200, "OK", filePath, MimeTypeResolver::resolveMimeType(filePath), HttpResponse::SUCCESS);
	return (true);
}

bool GetMethodHandler::_serveSymlink(const std::string &symlinkPath, HttpResponse &response, const Location &location)
{
	std::string resolvedPath = FileUtils::traverseSymlink(symlinkPath);
	if (resolvedPath.empty())
	{
		switch (errno)
		{
		case ENOENT:
			response.setResponseDefaultBody(404, "Not Found: Target does not exist", &location, HttpResponse::ERROR);
			break;
		case EACCES:
			response.setResponseDefaultBody(403, "Forbidden: Permission denied", &location, HttpResponse::ERROR);
			break;
		case ENOTDIR:
			response.setResponseDefaultBody(404, "Not Found: Invalid path component", &location, HttpResponse::ERROR);
			break;
		case ELOOP:
			response.setResponseDefaultBody(403, "Forbidden: Too many symlinks", &location, HttpResponse::ERROR);
			break;
		case ENAMETOOLONG:
			response.setResponseDefaultBody(414, "Path Too Long", &location, HttpResponse::ERROR);
			break;
		default:
			response.setResponseDefaultBody(500, "Internal Server Error: Unable to resolve symlink", &location,
											HttpResponse::ERROR);
			break;
		}
		return false;
	}
	else if (!FileUtils::inRoot(*location.getRootPath(), resolvedPath))
	{
		response.setResponseDefaultBody(403, "Forbidden: Symlink points outside of root directory", &location,
										HttpResponse::ERROR);
		return false;
	}
	return (_serveFile(resolvedPath, response, location));
}

bool GetMethodHandler::_serveDirectory(const HttpRequest &request, HttpResponse &response, const Location &location)
{
	std::string dirPath = request.getURI().getResolvedPath();
	if (dirPath.empty() || dirPath[dirPath.size() - 1] != '/')
	{
		// Redirect to path with trailing slash
		response.setRedirectResponse(301, "Moved Permanently: Directory path must end with '/'", dirPath + "/",
									 HttpResponse::SUCCESS);
		return (true);
	}
	if (location.hasIndexDirective())
	{
		const std::vector<std::string> &indexes = location.getIndexes()->getAllValues();
		for (std::vector<std::string>::const_iterator it = indexes.begin(); it != indexes.end(); ++it)
		{
			std::string indexPath = dirPath + "/" + *it;
			if (FileUtils::fileExists(indexPath))
			{
				return (_serveFile(indexPath, response, location));
			}
		}
	}
	if (location.hasAutoIndexDirective() && location.getAutoIndexValue())
	{
		std::string listing = _generateDirectoryListing(dirPath);
		response.setResponseCustomBody(200, "OK", listing, "text/html", HttpResponse::SUCCESS);
		return (true);
	}
	response.setResponseDefaultBody(403, "Forbidden", &location, HttpResponse::ERROR);
	return (false);
}

bool GetMethodHandler::handleRequest(const HttpRequest &request, HttpResponse &response, const Location &location)
{
	// Get sanitized file path
	std::string filePath = request.getURI().getResolvedPath();

	// Directory path handling
	switch (FileUtils::getFileType(filePath))
	{
	case FileUtils::NOT_FOUND: // NOT_FOUND
		response.setResponseDefaultBody(404, "Not Found", &location, HttpResponse::ERROR);
		break;
	case FileUtils::DIRECTORY:
		return _serveDirectory(request, response, location);
	case FileUtils::REGULAR_FILE:
		return _serveFile(filePath, response, location);
	case FileUtils::SYMBOLIC_LINK:
		return _serveSymlink(filePath, response, location);
	case FileUtils::CHARACTER_DEVICE:
		response.setResponseDefaultBody(403, "Forbidden: cannot access character devices", &location,
										HttpResponse::ERROR);
		break;
	case FileUtils::BLOCK_DEVICE:
		response.setResponseDefaultBody(403, "Forbidden: cannot access block devices", &location, HttpResponse::ERROR);
		break;
	case FileUtils::FIFO_PIPE:
		response.setResponseDefaultBody(403, "Forbidden: cannot access FIFO pipes", &location, HttpResponse::ERROR);
		break;
	case FileUtils::SOCKET:
		response.setResponseDefaultBody(403, "Forbidden: cannot access sockets", &location, HttpResponse::ERROR);
		break;
	default:
		response.setResponseDefaultBody(500, "Internal Server Error: unknown file type", &location,
										HttpResponse::ERROR);
		break;
	}
	return (false);
}
