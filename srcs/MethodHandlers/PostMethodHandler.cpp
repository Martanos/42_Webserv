#include "PostMethodHandler.hpp"

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

void PostMethodHandler::handle(const HttpRequest &request,
							   HttpResponse &response,
							   const Server *server,
							   const Location *location)
{
	// The request body is already parsed by HttpRequest
	// Check content length against server limits
	size_t contentLength = request.getContentLength();
	if (contentLength > static_cast<size_t>(server->getClientMaxBodySize()))
	{
		response.setStatus(413, "Payload Too Large");
		response.setBody(server->getStatusPage(413));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length",
						   StringUtils::toString(response.getBody().length()));
		return;
	}

	// Check if location allows POST
	if (location)
	{
		const std::vector<std::string> &methods = location->getAllowedMethods();
		bool postAllowed = false;
		for (size_t i = 0; i < methods.size(); ++i)
		{
			if (methods[i] == "POST")
			{
				postAllowed = true;
				break;
			}
		}

		if (!postAllowed)
		{
			response.setStatus(405, "Method Not Allowed");
			response.setBody(server->getStatusPage(405));
			response.setHeader("Content-Type", "text/html");
			response.setHeader("Content-Length",
							   StringUtils::toString(response.getBody().length()));
			response.setHeader("Allow", "GET, HEAD");
			return;
		}
	}

	// Check what type of POST handling is configured
	if (location && !location->getCgiPath().empty())
	{
		handleCGI(request, response, location, server);
	}
	else if (location && !location->getUploadPath().empty())
	{
		handleFileUpload(request, response, location, server);
	}
	else
	{
		// No specific POST handler configured - return error
		response.setStatus(501, "Not Implemented");
		response.setBody(server->getStatusPage(501));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length",
						   StringUtils::toString(response.getBody().length()));
	}
}

void PostMethodHandler::handleFileUpload(const HttpRequest &request,
										 HttpResponse &response,
										 const Location *location,
										 const Server *server) const
{
	std::string uploadPath = location->getUploadPath();

	// Verify upload directory exists and is writable
	struct stat st;
	if (stat(uploadPath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
	{
		Logger::log(Logger::ERROR, "Upload directory not accessible: " + uploadPath);
		response.setStatus(500, "Internal Server Error");
		response.setBody(server->getStatusPage(500));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length",
						   StringUtils::toString(response.getBody().length()));
		return;
	}

	// The body is already parsed, just save it
	// Generate filename based on timestamp if not provided
	std::stringstream ss;
	ss << "upload_" << time(NULL) << "_" << rand() % 10000;

	// Check Content-Type for filename hint
	std::string contentType = request.getHeader("content-type");
	std::string filename = ss.str();

	// If it's a form upload, try to extract filename from Content-Disposition
	std::string contentDisposition = request.getHeader("content-disposition");
	if (!contentDisposition.empty())
	{
		std::string extractedName = extractFilename(contentDisposition);
		if (!extractedName.empty())
		{
			filename = extractedName;
		}
	}
	else if (contentType.find("application/octet-stream") != std::string::npos)
	{
		filename += ".bin";
	}
	else if (contentType.find("text/plain") != std::string::npos)
	{
		filename += ".txt";
	}
	else if (contentType.find("image/") != std::string::npos)
	{
		size_t pos = contentType.find("image/");
		std::string ext = contentType.substr(pos + 6);
		size_t semicolon = ext.find(';');
		if (semicolon != std::string::npos)
		{
			ext = ext.substr(0, semicolon);
		}
		filename += "." + ext;
	}
	else
	{
		filename += ".dat";
	}

	// Save the file
	if (saveUploadedFile(uploadPath, filename, request.getBody()))
	{
		// Success response
		std::stringstream html;
		html << "<!DOCTYPE html>\n<html>\n<head>\n";
		html << "<title>Upload Successful</title>\n</head>\n<body>\n";
		html << "<h1>File Uploaded Successfully</h1>\n";
		html << "<p>Filename: " << filename << "</p>\n";
		html << "<p>Size: " << request.getBody().length() << " bytes</p>\n";
		html << "<p><a href=\"" << request.getUri() << "\">Back</a></p>\n";
		html << "</body>\n</html>\n";

		response.setStatus(201, "Created");
		response.setBody(html.str());
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length",
						   StringUtils::toString(response.getBody().length()));
		response.setHeader("Location", uploadPath + "/" + filename);
	}
	else
	{
		// Failed to save
		response.setStatus(500, "Internal Server Error");
		response.setBody(server->getStatusPage(500));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length",
						   StringUtils::toString(response.getBody().length()));
	}
}

// TODO: Have more robust file name generation
bool PostMethodHandler::saveUploadedFile(const std::string &uploadPath,
										 const std::string &filename,
										 const std::string &content) const
{
	std::string fullPath = uploadPath;
	if (fullPath[fullPath.length() - 1] != '/')
	{
		fullPath += '/';
	}
	fullPath += filename;

	// Use FileDescriptor for safe file handling
	FileDescriptor fd(open(fullPath.c_str(),
						   O_WRONLY | O_CREAT | O_EXCL,
						   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));

	if (!fd.isValid())
	{
		// Try with a numbered suffix if file exists
		for (int i = 1; i < 100; ++i)
		{
			std::stringstream newPath;
			size_t dotPos = filename.find_last_of('.');
			if (dotPos != std::string::npos)
			{
				newPath << uploadPath << "/"
						<< filename.substr(0, dotPos)
						<< "_" << i
						<< filename.substr(dotPos);
			}
			else
			{
				newPath << uploadPath << "/" << filename << "_" << i;
			}

			fd.setFd(open(newPath.str().c_str(),
						  O_WRONLY | O_CREAT | O_EXCL,
						  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));

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

// TODO: Implement CGI handling
void PostMethodHandler::handleCGI(const HttpRequest &request,
								  HttpResponse &response,
								  const Location *location,
								  const Server *server) const
{
	// Simplified CGI handling placeholder
	// The full implementation would:
	// 1. Set up environment variables from request headers
	// 2. Create pipes for stdin/stdout
	// 3. Fork process
	// 4. Execute CGI script with request body as stdin
	// 5. Read CGI output and parse headers
	// 6. Return response

	(void)request;
	(void)server;
	(void)location;

	response.setStatus(501, "Not Implemented");
	response.setBody("<h1>CGI Support Not Yet Implemented</h1>");
	response.setHeader("Content-Type", "text/html");
	response.setHeader("Content-Length",
					   StringUtils::toString(response.getBody().length()));
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
