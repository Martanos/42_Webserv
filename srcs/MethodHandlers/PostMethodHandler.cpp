#include "../../includes/PostMethodHandler.hpp"
#include "../../includes/PerformanceMonitor.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

PostMethodHandler::PostMethodHandler()
{
}

PostMethodHandler::PostMethodHandler(const PostMethodHandler &src)
{
	(void)src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

PostMethodHandler::~PostMethodHandler()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

PostMethodHandler &PostMethodHandler::operator=(PostMethodHandler const &rhs)
{
	(void)rhs;
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

bool PostMethodHandler::saveUploadedFile(const std::string &uploadPath, const std::string &filename,
										 const std::string &content) const
{
	std::string fullPath = uploadPath;
	if (fullPath[fullPath.length() - 1] != '/')
	{
		fullPath += '/';
	}

	// Generate robust filename with timestamp and random component
	std::string safeFilename = generateSafeFilename(filename);
	fullPath += safeFilename;

	// Use FileDescriptor for safe file handling
	FileDescriptor fd = FileDescriptor::createFromOpen(fullPath.c_str(), O_WRONLY | O_CREAT | O_EXCL,
													   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (!fd.isValid())
	{
		// Try with a numbered suffix if file exists
		for (int i = 1; i < 100; ++i)
		{
			std::stringstream newPath;
			size_t dotPos = filename.find_last_of('.');
			if (dotPos != std::string::npos)
			{
				newPath << uploadPath << "/" << filename.substr(0, dotPos) << "_" << i << filename.substr(dotPos);
			}
			else
			{
				newPath << uploadPath << "/" << filename << "_" << i;
			}

			fd = FileDescriptor::createFromOpen(newPath.str().c_str(), O_WRONLY | O_CREAT | O_EXCL,
												S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (fd.isValid())
			{
				break;
			}
		}

		if (!fd.isValid())
		{
			Logger::log(Logger::ERROR, "Cannot create upload file: " + fullPath);
			return false;
		}
	}

	// Write content
	ssize_t written = write(fd.getFd(), content.c_str(), content.length());
	if (written < 0 || static_cast<size_t>(written) != content.length())
	{
		Logger::log(Logger::ERROR, "Failed to write complete file: " + fullPath);
		return false;
	}

	// FileDescriptor destructor will close the file
	return true;
}

std::string PostMethodHandler::extractFilename(const std::string &contentDisposition) const
{
	size_t pos = contentDisposition.find("filename=\"");
	if (pos != std::string::npos)
	{
		pos += 10; // Length of "filename=\""
		size_t endPos = contentDisposition.find("\"", pos);
		if (endPos != std::string::npos)
		{
			return contentDisposition.substr(pos, endPos - pos);
		}
	}
	return "";
}

void PostMethodHandler::handleCGI(const HttpRequest &request, HttpResponse &response, const Location *location,
								  const Server *server) const
{
	// Use the new CgiHandler for CGI execution
	CgiHandler cgiHandler;

	CgiHandler::ExecutionResult result = cgiHandler.execute(request, response, server, location);

	if (result != CgiHandler::SUCCESS)
	{
		// CGI execution failed, return appropriate error
		int statusCode = 500;
		std::string statusMessage = "Internal Server Error";

		switch (result)
		{
		case CgiHandler::ERROR_SCRIPT_NOT_FOUND:
			statusCode = 404;
			statusMessage = "Not Found";
			break;
		case CgiHandler::ERROR_TIMEOUT:
			statusCode = 504;
			statusMessage = "Gateway Timeout";
			break;
		case CgiHandler::ERROR_INVALID_SCRIPT_PATH:
			statusCode = 400;
			statusMessage = "Bad Request";
			break;
		default:
			statusCode = 500;
			statusMessage = "Internal Server Error";
			break;
		}

		response.setStatus(statusCode, statusMessage);
		response.setBody(server->getStatusPage(statusCode));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
	}
}

std::string PostMethodHandler::generateSafeFilename(const std::string &originalFilename) const
{
	std::string safeFilename = originalFilename;

	// Remove or replace dangerous characters
	for (size_t i = 0; i < safeFilename.length(); ++i)
	{
		char c = safeFilename[i];
		if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
		{
			safeFilename[i] = '_';
		}
	}

	// Add timestamp to make filename unique
	std::time_t now = std::time(0);
	std::stringstream ss;
	ss << std::time(&now) << "_" << safeFilename;

	return ss.str();
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

bool PostMethodHandler::canHandle(const std::string &method) const
{
	return method == "POST";
}

// Get the method name
std::string PostMethodHandler::getMethod() const
{
	return "POST";
}

/* ************************************************************************** */
