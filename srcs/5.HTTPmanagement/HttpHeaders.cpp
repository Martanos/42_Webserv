#include "../../includes/HttpHeaders.hpp"
#include "../../includes/HttpBody.hpp"
#include "../../includes/HttpResponse.hpp"
#include "../../includes/Logger.hpp"
#include "../../includes/StringUtils.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpHeaders::HttpHeaders() : _headersState(HEADERS_PARSING), _expectedBodySize(0), _rawHeaders(), _headers()
{
}

HttpHeaders::HttpHeaders(HttpHeaders const &src) : _headersState(src._headersState), 
	_expectedBodySize(src._expectedBodySize), _rawHeaders(src._rawHeaders), _headers(src._headers)
{
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
		_expectedBodySize = rhs._expectedBodySize;
		_rawHeaders = rhs._rawHeaders;
		_headers = rhs._headers;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

int HttpHeaders::parseBuffer(RingBuffer &buffer, HttpResponse &response, HttpBody &body)
{
	if (_headersState == HEADERS_PARSING_COMPLETE)
		return HEADERS_PARSING_COMPLETE;

	if (_headersState == HEADERS_PARSING_ERROR)
		return HEADERS_PARSING_ERROR;

	// Transfer data from buffer to raw headers
	size_t transferred = _rawHeaders.transferFrom(buffer, buffer.readable());
	if (transferred == 0)
		return HEADERS_PARSING;

	// Look for end of headers (CRLF CRLF or LF LF)
	std::string rawData;
	_rawHeaders.peekBuffer(rawData, _rawHeaders.readable());

	size_t headerEnd = rawData.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
	{
		headerEnd = rawData.find("\n\n");
		if (headerEnd == std::string::npos)
		{
			// Still need more data
			return HEADERS_PARSING;
		}
		headerEnd += 2; // Include the \n\n
	}
	else
	{
		headerEnd += 4; // Include the \r\n\r\n
	}

	// Extract headers
	std::string headersData;
	_rawHeaders.readBuffer(headersData, headerEnd);

	// Parse headers
	parseHeaders(response, body);

	_headersState = HEADERS_PARSING_COMPLETE;
	return HEADERS_PARSING_COMPLETE;
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
	headerName = headerName.substr(headerName.find_first_not_of(" \t\r\n"), 
		headerName.find_last_not_of(" \t\r\n") - headerName.find_first_not_of(" \t\r\n") + 1);
	headerValue = headerValue.substr(headerValue.find_first_not_of(" \t\r\n"), 
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
		long contentLength = std::strtol(headerValue.c_str(), &endPtr, 10);
		if (*endPtr != '\0' || contentLength < 0)
		{
			Logger::log(Logger::WARNING, "Invalid Content-Length header: " + headerValue);
			_headersState = HEADERS_PARSING_ERROR;
			response.setStatus(400, "Bad Request");
			return;
		}
		_expectedBodySize = static_cast<size_t>(contentLength);
		body.setExpectedBodySize(_expectedBodySize);
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

void HttpHeaders::parseHeaders(HttpResponse &response, HttpBody &body)
{
	std::string headersData;
	_rawHeaders.readBuffer(headersData, _rawHeaders.readable());

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

/*
** --------------------------------- ACCESSORS --------------------------------
*/

int HttpHeaders::getHeadersState() const
{
	return _headersState;
}

RingBuffer HttpHeaders::getRawHeaders() const
{
	return _rawHeaders;
}

std::map<std::string, std::vector<std::string> > HttpHeaders::getHeaders() const
{
	return _headers;
}

size_t HttpHeaders::getHeadersSize() const
{
	size_t size = 0;
	for (std::map<std::string, std::vector<std::string> >::const_iterator it = _headers.begin(); 
		 it != _headers.end(); ++it)
	{
		size += it->first.length();
		for (std::vector<std::string>::const_iterator vit = it->second.begin(); 
			 vit != it->second.end(); ++vit)
		{
			size += vit->length();
		}
	}
	return size;
}

size_t HttpHeaders::getExpectedBodySize() const
{
	return _expectedBodySize;
}

/*
** --------------------------------- MUTATORS --------------------------------
*/

void HttpHeaders::setHeadersState(HeadersState headersState)
{
	_headersState = headersState;
}

void HttpHeaders::setRawHeaders(const RingBuffer &rawHeaders)
{
	_rawHeaders = rawHeaders;
}

void HttpHeaders::setHeaders(const std::map<std::string, std::vector<std::string> > &headers)
{
	_headers = headers;
}

void HttpHeaders::setExpectedBodySize(size_t expectedBodySize)
{
	_expectedBodySize = expectedBodySize;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpHeaders::reset()
{
	_headersState = HEADERS_PARSING;
	_expectedBodySize = 0;
	_rawHeaders.clear();
	_headers.clear();
}