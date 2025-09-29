#include "../../includes/HttpHeaders.hpp"
#include "../../includes/HttpBody.hpp"
#include "../../includes/HttpResponse.hpp"
#include "../../includes/Logger.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpHeaders::HttpHeaders()
{
	_headersState = HEADERS_PARSING;
	_headers = std::map<std::string, std::vector<std::string> >();
	_expectedBodySize = 0;
}

HttpHeaders::HttpHeaders(HttpHeaders const &src)
{
	*this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

HttpHeaders::~HttpHeaders()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

HttpHeaders &HttpHeaders::operator=(HttpHeaders const &rhs)
{
	if (this != &rhs)
	{
		_headersState = rhs._headersState;
		_headers = rhs._headers;
		_expectedBodySize = rhs._expectedBodySize;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpHeaders::parseBuffer(std::vector<char> &buffer, HttpResponse &response, HttpBody &body)
{
	// Scan buffer for CLRF
	const char *pattern = "\r\n\r\n";
	std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), pattern, pattern + 4);
	if (it == buffer.end())
	{
		// If it can't be found check that the buffer has not currently exceeded the size limit of a header
		if (buffer.size() > static_cast<size_t>(sysconf(_SC_PAGESIZE) * 4))
		{
			response.setStatus(413, "Request Header Fields Too Large");
			Logger::log(Logger::ERROR, "Header size limit exceeded");
			_headersState = HEADERS_PARSING_ERROR;
		}
		else
			_headersState = HEADERS_PARSING;
		return;
	}

	// Extract header data from buffer
	std::string headersData;
	headersData.assign(buffer.begin(), it);

	// Clear buffer up to the CLRF
	buffer.erase(buffer.begin(), it + 4);

	// Parse headers
	parseHeaders(headersData, response, body);
}

// Using a string stream to parse the headers into a map of headers
void HttpHeaders::parseHeaders(const std::string &headersData, HttpResponse &response, HttpBody &body)
{
	if (headersData.empty())
		return;

	std::istringstream stream(headersData);
	std::string line;

	while (std::getline(stream, line))
	{
		// Remove trailing CRLF or LF
		if (!line.empty() && line[line.length() - 1] == '\r')
		{
			line = line.substr(0, line.length() - 1);
		}

		parseHeaderLine(line, response, body);
		if (_headersState == HEADERS_PARSING_ERROR)
			return;
	}
}

void HttpHeaders::parseHeaderLine(const std::string &line, HttpResponse &response, HttpBody &body)
{
	if (line.empty())
		return;

	// Find colon separator
	size_t colonPos = line.find(':');
	if (colonPos == std::string::npos)
	{
		Logger::log(Logger::WARNING, "Invalid header line (no colon): " + line);
		_headersState = HEADERS_PARSING_ERROR;
		response.setStatus(400, "Bad Request");
		return;
	}

	// Extract header name and value
	std::string headerName = line.substr(0, colonPos);
	std::string headerValue = line.substr(colonPos + 1);

	// Trim whitespace
	headerName =
		headerName.substr(headerName.find_first_not_of(" \t\r\n"),
						  headerName.find_last_not_of(" \t\r\n") - headerName.find_first_not_of(" \t\r\n") + 1);
	headerValue =
		headerValue.substr(headerValue.find_first_not_of(" \t\r\n"),
						   headerValue.find_last_not_of(" \t\r\n") - headerValue.find_first_not_of(" \t\r\n") + 1);

	// Convert header name to lowercase
	std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);

	// Validate header name (no spaces allowed)
	if (headerName.find(' ') != std::string::npos)
	{
		Logger::log(Logger::WARNING, "Invalid header name (contains spaces): " + headerName);
		_headersState = HEADERS_PARSING_ERROR;
		response.setStatus(400, "Bad Request");
		return;
	}

	// Store header
	_headers[headerName].push_back(headerValue);

	// Handle special headers
	if (headerName == "content-length")
	{
		char *endPtr;
		ssize_t contentLength = std::strtol(headerValue.c_str(), &endPtr, 10);
		if (*endPtr != '\0' || contentLength < 0)
		{
			Logger::log(Logger::WARNING, "Invalid Content-Length header: " + headerValue);
			_headersState = HEADERS_PARSING_ERROR;
			response.setStatus(400, "Bad Request");
			return;
		}
		_expectedBodySize = contentLength;
		body.setExpectedBodySize(contentLength);
		body.setBodyType(HttpBody::BODY_TYPE_CONTENT_LENGTH);
	}
	else if (headerName == "transfer-encoding")
	{
		std::transform(headerValue.begin(), headerValue.end(), headerValue.begin(), ::tolower);
		if (headerValue.find("chunked") != std::string::npos)
		{
			body.setBodyType(HttpBody::BODY_TYPE_CHUNKED);
		}
	}
	else if (headerName == "connection")
	{
		std::transform(headerValue.begin(), headerValue.end(), headerValue.begin(), ::tolower);
		if (headerValue == "close")
		{
			response.setHeader("Connection", "close");
		}
		else if (headerValue == "keep-alive")
		{
			response.setHeader("Connection", "keep-alive");
		}
	}
	else if (headerName == "host")
	{
		// Host header is required for HTTP/1.1
		if (headerValue.empty())
		{
			Logger::log(Logger::WARNING, "Empty Host header");
			_headersState = HEADERS_PARSING_ERROR;
			response.setStatus(400, "Bad Request");
			return;
		}
	}
}

/*
** --------------------------------- ACCESSORS --------------------------------
*/

int HttpHeaders::getHeadersState() const
{
	return _headersState;
}

std::map<std::string, std::vector<std::string> > HttpHeaders::getHeaders() const
{
	return _headers;
}

size_t HttpHeaders::getExpectedBodySize() const
{
	return _expectedBodySize;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpHeaders::reset()
{
	_headersState = HEADERS_PARSING;
	_headers.clear();
	_expectedBodySize = 0;
}