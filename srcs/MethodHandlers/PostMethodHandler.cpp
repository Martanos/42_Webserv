#include "PostMethodHandler.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

PostMethodHandler::PostMethodHandler()
{
	throw std::runtime_error("PostMethodHandler constructor not implemented");
}

PostMethodHandler::PostMethodHandler(const PostMethodHandler &src)
{
	(void)src;
	throw std::runtime_error("PostMethodHandler constructor not implemented");
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
	throw std::runtime_error("PostMethodHandler assignment operator not implemented");
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

// Main handler method
void PostMethodHandler::handle(const HttpRequest &request,
							   HttpResponse &response,
							   const Server *server,
							   const Location *location)
{
	try
	{
		// Validate content length
		size_t contentLength = request.getContentLength();
		if (!validateContentLength(contentLength, server))
		{
			response.setStatus(413, "Payload Too Large");
			response.setBody(server->getStatusPage(413));
			response.setHeader("Content-Type", "text/html");
			response.setHeader("Content-Length",
							   StringUtils::toString(response.getBody().length()));
			return;
		}

		// Check if this location has CGI configured
		if (location && !location->getCgiPath().empty())
		{
			handleCGI(request, response, location, server);
			return;
		}

		// Check if this location has upload path configured
		if (location && !location->getUploadPath().empty())
		{
			handleFileUpload(request, response, location, server);
			return;
		}

		// No handler configured for POST at this location
		response.setStatus(405, "Method Not Allowed");
		response.setBody(server->getStatusPage(405));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length",
						   StringUtils::toString(response.getBody().length()));
		response.setHeader("Allow", "GET, HEAD");
	}
	catch (const std::exception &e)
	{
		Logger::log(Logger::ERROR, "Error handling POST request: " + std::string(e.what()));
		response.setStatus(500, "Internal Server Error");
		response.setBody(server->getStatusPage(500));
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

	// Ensure upload directory exists
	struct stat st;
	if (stat(uploadPath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
	{
		Logger::log(Logger::ERROR, "Upload directory does not exist: " + uploadPath);
		response.setStatus(500, "Internal Server Error");
		response.setBody(server->getStatusPage(500));
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length",
						   StringUtils::toString(response.getBody().length()));
		return;
	}

	// Get Content-Type header
	std::string contentType = request.getHeader("content-type");

	// Check if it's multipart form data
	if (contentType.find("multipart/form-data") != std::string::npos)
	{
		// Extract boundary
		std::string boundary = extractBoundary(contentType);
		if (boundary.empty())
		{
			response.setStatus(400, "Bad Request");
			response.setBody(server->getStatusPage(400));
			response.setHeader("Content-Type", "text/html");
			response.setHeader("Content-Length",
							   StringUtils::toString(response.getBody().length()));
			return;
		}

		// Parse multipart data
		std::string filename;
		std::string fileContent;
		if (!parseMultipartData(request.getBody(), boundary, filename, fileContent))
		{
			response.setStatus(400, "Bad Request");
			response.setBody(server->getStatusPage(400));
			response.setHeader("Content-Type", "text/html");
			response.setHeader("Content-Length",
							   StringUtils::toString(response.getBody().length()));
			return;
		}

		// Generate unique filename
		std::string finalPath = generateUniqueFilename(uploadPath, filename);

		// Write file
		std::ofstream file(finalPath.c_str(), std::ios::binary);
		if (!file.is_open())
		{
			Logger::log(Logger::ERROR, "Cannot create file: " + finalPath);
			response.setStatus(500, "Internal Server Error");
			response.setBody(server->getStatusPage(500));
			response.setHeader("Content-Type", "text/html");
			response.setHeader("Content-Length",
							   StringUtils::toString(response.getBody().length()));
			return;
		}

		file.write(fileContent.c_str(), fileContent.length());
		file.close();

		// Generate success response
		std::stringstream html;
		html << "<!DOCTYPE html>\n<html>\n<head>\n";
		html << "<title>Upload Successful</title>\n</head>\n<body>\n";
		html << "<h1>File Uploaded Successfully</h1>\n";
		html << "<p>File: " << filename << "</p>\n";
		html << "<p>Size: " << fileContent.length() << " bytes</p>\n";
		html << "<p><a href=\"" << request.getUri() << "\">Back</a></p>\n";
		html << "</body>\n</html>\n";

		std::string htmlContent = html.str();
		response.setStatus(201, "Created");
		response.setBody(htmlContent);
		response.setHeader("Content-Type", "text/html");
		response.setHeader("Content-Length", StringUtils::toString(htmlContent.length()));
	}
	else
	{
		// Handle raw POST data (save as unnamed file)
		std::string filename = "upload_" + StringUtils::toString(time(NULL)) + ".dat";
		std::string finalPath = uploadPath + "/" + filename;

		std::ofstream file(finalPath.c_str(), std::ios::binary);
		if (!file.is_open())
		{
			Logger::log(Logger::ERROR, "Cannot create file: " + finalPath);
			response.setStatus(500, "Internal Server Error");
			response.setBody(server->getStatusPage(500));
			response.setHeader("Content-Type", "text/html");
			response.setHeader("Content-Length",
							   StringUtils::toString(response.getBody().length()));
			return;
		}

		file.write(request.getBody().c_str(), request.getBody().length());
		file.close();

		response.setStatus(201, "Created");
		response.setBody("{\"status\":\"success\",\"file\":\"" + filename + "\"}");
		response.setHeader("Content-Type", "application/json");
		response.setHeader("Content-Length",
						   StringUtils::toString(response.getBody().length()));
	}
}

// Handle CGI execution
void PostMethodHandler::handleCGI(const HttpRequest &request,
								  HttpResponse &response,
								  const Location *location,
								  const Server *server) const
{
	// This is a simplified CGI handler - you'll need to expand this
	// based on your CGI requirements

	(void)request;
	(void)server;

	std::string cgiPath = location->getCgiPath();

	// Check if CGI executable exists
	struct stat st;
	if (stat(cgiPath.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
	{
		Logger::log(Logger::ERROR, "CGI executable not found: " + cgiPath);
		response.setStatus(500, "Internal Server Error");
		return;
	}

	// Check if executable
	if (!(st.st_mode & S_IXUSR))
	{
		Logger::log(Logger::ERROR, "CGI not executable: " + cgiPath);
		response.setStatus(500, "Internal Server Error");
		return;
	}

	// TODO: Implement full CGI execution
	// This would involve:
	// 1. Setting up environment variables
	// 2. Creating pipes for stdin/stdout
	// 3. Forking a process
	// 4. Executing the CGI script
	// 5. Reading the output
	// 6. Parsing CGI headers
	// 7. Returning the response

	response.setStatus(501, "Not Implemented");
	response.setBody("<h1>CGI Support Coming Soon</h1>");
	response.setHeader("Content-Type", "text/html");
	response.setHeader("Content-Length",
					   StringUtils::toString(response.getBody().length()));
}

// Parse multipart form data
bool PostMethodHandler::parseMultipartData(const std::string &body,
										   const std::string &boundary,
										   std::string &filename,
										   std::string &fileContent) const
{
	// Find the boundary in the body
	std::string delimiter = "--" + boundary;
	size_t pos = body.find(delimiter);
	if (pos == std::string::npos)
	{
		return false;
	}

	// Skip to after first boundary
	pos += delimiter.length();

	// Find Content-Disposition header
	size_t headerEnd = body.find("\r\n\r\n", pos);
	if (headerEnd == std::string::npos)
	{
		return false;
	}

	std::string headers = body.substr(pos, headerEnd - pos);

	// Extract filename from Content-Disposition
	size_t filenamePos = headers.find("filename=\"");
	if (filenamePos != std::string::npos)
	{
		filenamePos += 10; // Length of "filename=\""
		size_t filenameEnd = headers.find("\"", filenamePos);
		if (filenameEnd != std::string::npos)
		{
			filename = headers.substr(filenamePos, filenameEnd - filenamePos);
		}
	}

	if (filename.empty())
	{
		filename = "unnamed_file";
	}

	// Extract file content
	size_t contentStart = headerEnd + 4; // Skip \r\n\r\n
	size_t contentEnd = body.find("\r\n" + delimiter, contentStart);
	if (contentEnd == std::string::npos)
	{
		contentEnd = body.find("\r\n" + delimiter + "--", contentStart);
		if (contentEnd == std::string::npos)
		{
			return false;
		}
	}

	fileContent = body.substr(contentStart, contentEnd - contentStart);

	return true;
}

// Validate content length against server limits
bool PostMethodHandler::validateContentLength(size_t contentLength,
											  const Server *server) const
{
	double maxSize = server->getClientMaxBodySize();
	return contentLength <= static_cast<size_t>(maxSize);
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
