<<<<<<< HEAD
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
=======
#include "../../includes/HTTP/HttpHeaders.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/HTTP/Constants.hpp"
#include "../../includes/HTTP/HttpBody.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpHeaders::HttpHeaders()
{
	_headersState = HEADERS_PARSING;
	_headers = std::map<std::string, std::vector<std::string> >();
	_rawHeadersSize = 0;
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
		_rawHeadersSize = rhs._rawHeadersSize;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpHeaders::parseBuffer(std::vector<char> &buffer, HttpResponse &response, HttpBody &body)
{
	std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), HTTP::CRLF, HTTP::CRLF + 2);
	if (it == buffer.end())
	{
		// If it can't be found check that the buffer has not currently exceeded the size limit of a header
		if (buffer.size() > HTTP::MAX_HEADERS_LINE_SIZE)
		{
			response.setStatus(413, "Request Header Too Large");
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
	buffer.erase(buffer.begin(), it + 2);
	if (headersData.empty())
	{
		parseAllHeaders(response, body);
		_headersState = HEADERS_PARSING_COMPLETE;
		Logger::log(Logger::DEBUG, "Headers parsing complete");
		return;
	}
	else if (headersData.size() + 2 > HTTP::MAX_HEADERS_LINE_SIZE)
	{
		response.setStatus(413, "Request Line Header Too Large");
		Logger::log(Logger::ERROR, "Header line size limit exceeded");
		_headersState = HEADERS_PARSING_ERROR;
		return;
	}
	_rawHeadersSize = headersData.size() + 2;
	if (_rawHeadersSize > HTTP::MAX_HEADERS_SIZE)
	{
		response.setStatus(413, "Request headers total size too large");
		Logger::log(Logger::ERROR, "Header total size limit exceeded");
		_headersState = HEADERS_PARSING_ERROR;
		return;
	}

	// Parse headers
	parseHeaderLine(headersData, response);
}

void HttpHeaders::parseHeaderLine(const std::string &line, HttpResponse &response)
{
	// Find colon separator
	size_t colonPos = line.find(':');
	if (colonPos == std::string::npos)
	{
		Logger::log(Logger::WARNING, "Invalid header line (no colon): " + line);
		_headersState = HEADERS_PARSING_ERROR;
		response.setStatus(400, "Bad Request");
		return;
	}
	// Extract and sanitize header name
	std::string headerName = line.substr(0, colonPos);
	if (headerName.empty())
	{
		Logger::log(Logger::WARNING, "Empty header name");
		_headersState = HEADERS_PARSING_ERROR;
		response.setStatus(400, "Bad Request");
		return;
	}
	for (std::string::iterator it = headerName.begin(); it != headerName.end(); ++it)
	{
		unsigned char c = static_cast<unsigned char>(*it);
		bool isValid = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '!' ||
					   c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' || c == '*' || c == '+' || c == '-' ||
					   c == '.' || c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
		if (!isValid)
		{
			Logger::log(Logger::WARNING, "Invalid header name (contains non-alphanumeric characters): " + headerName);
			_headersState = HEADERS_PARSING_ERROR;
			response.setStatus(400, "Bad Request");
			return;
		}
	}

	std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);

	// TODO: Divide this into , and ; cases
	// Extract and sanitize values
	std::string headerValue = line.substr(colonPos + 1);
	std::vector<std::string> headerValues;
	std::stringstream headerValueStream(headerValue);
	std::string value;
	while (std::getline(headerValueStream, value, ','))
	{
		value = value.substr(value.find_first_not_of(" \t\r\n"),
							 value.find_last_not_of(" \t\r\n") - value.find_first_not_of(" \t\r\n") + 1);
		if (!value.empty())
			headerValues.push_back(value);
	}

	// Singleton header handling
	if (isSingletonHeader(headerName) && !headerValues.empty())
	{
		if (_headers.find(headerName) != _headers.end())
		{
			Logger::error("Duplicate singleton header: " + headerName);
			_headersState = HEADERS_PARSING_ERROR;
			response.setStatus(400, "Bad Request");
			return;
		}
		else if (headerValues.size() != 1)
		{
			Logger::error("Invalid number of values for singleton header: " + headerName);
			_headersState = HEADERS_PARSING_ERROR;
			response.setStatus(400, "Bad Request");
			return;
		}
	}

	// If everything is good, store the header
	if (_headers.find(headerName) == _headers.end())
	{
		_headers[headerName] = headerValues;
	}
	else
	{
		_headers[headerName].insert(_headers[headerName].end(), headerValues.begin(), headerValues.end());
	}
	return;
}

// Checks all headers for special characters
void HttpHeaders::parseAllHeaders(HttpResponse &response, HttpBody &body)
{
	bool hostFound = false;
	for (std::map<std::string, std::vector<std::string> >::iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		std::string headerName = it->first;
		std::vector<std::string> headerValues = it->second;
		if (headerName == "content-length")
		{
			if (_headers.find("transfer-encoding") != _headers.end())
			{
				Logger::log(Logger::WARNING, "Content-Length and Transfer-Encoding headers cannot be used together");
				_headersState = HEADERS_PARSING_ERROR;
				response.setStatus(400, "Bad Request");
				return;
			}
			char *endPtr;
			ssize_t contentLength = std::strtol(headerValues[0].c_str(), &endPtr, 10);
			if (*endPtr != '\0' || contentLength < 0)
			{
				Logger::log(Logger::WARNING, "Invalid Content-Length header: " + headerValues[0]);
				_headersState = HEADERS_PARSING_ERROR;
				response.setStatus(400, "Bad Request");
				return;
			}
			body.setExpectedBodySize(contentLength);
			body.setBodyType(HttpBody::BODY_TYPE_CONTENT_LENGTH);
		}
		else if (headerName == "transfer-encoding")
		{
			if (_headers.find("content-length") != _headers.end())
			{
				Logger::log(Logger::WARNING, "Content-Length and Transfer-Encoding headers cannot be used together");
				_headersState = HEADERS_PARSING_ERROR;
				response.setStatus(400, "Bad Request");
				return;
			}
			// As per requirements, only chunked is supported
			if (headerValues[0].find("chunked") != std::string::npos)
			{
				body.setBodyType(HttpBody::BODY_TYPE_CHUNKED);
			}
			else
			{
				Logger::log(Logger::WARNING, "Invalid Transfer-Encoding header: " + headerValues[0]);
				_headersState = HEADERS_PARSING_ERROR;
				response.setStatus(400, "Bad Request");
				return;
			}
		}
		else if (headerName == "connection")
		{
			std::transform(headerValues[0].begin(), headerValues[0].end(), headerValues[0].begin(), ::tolower);
			if (headerValues[0] == "close")
			{
				response.setHeader("Connection", "close");
			}
			else if (headerValues[0] == "keep-alive")
			{
				response.setHeader("Connection", "keep-alive");
			}
			else
			{
				Logger::log(Logger::WARNING, "Invalid Transfer-Encoding header: " + headerValues[0]);
				_headersState = HEADERS_PARSING_ERROR;
				response.setStatus(400, "Bad Request");
				return;
			}
		}
		else if (headerName == "host")
		{
			hostFound = true;
			// Host header is required for HTTP/1.1
			if (headerValues[0].empty())
			{
				Logger::log(Logger::WARNING, "Empty Host header");
				_headersState = HEADERS_PARSING_ERROR;
				response.setStatus(400, "Bad Request");
				return;
			}
		}
		else if (headerName == "content-disposition" || headerName == "referer" || headerName == "location")
		{
			for (std::vector<std::string>::iterator it = headerValues.begin(); it != headerValues.end(); ++it)
			{
				*it = HTTP_PARSING_UTILS::percentDecode(*it);
			}
		}
	}
	if (!hostFound)
	{
		Logger::error("Host header is required for HTTP/1.1");
		_headersState = HEADERS_PARSING_ERROR;
		response.setStatus(400, "Bad Request");
		return;
	}
}

bool HttpHeaders::isSingletonHeader(const std::string &headerName)
{
	std::vector<std::string> singletonHeaders(HTTP::SINGLETON_HEADERS,
											  HTTP::SINGLETON_HEADERS +
												  sizeof(HTTP::SINGLETON_HEADERS) / sizeof(HTTP::SINGLETON_HEADERS[0]));
	return std::find(singletonHeaders.begin(), singletonHeaders.end(), headerName) != singletonHeaders.end();
}

/*
** --------------------------------- ACCESSORS --------------------------------
*/

int HttpHeaders::getHeadersState() const
{
	return _headersState;
}

const std::map<std::string, std::vector<std::string> > &HttpHeaders::getHeaders() const
{
	return _headers;
}

const std::vector<std::string> HttpHeaders::getHeader(const std::string &headerName) const
{
	std::map<std::string, std::vector<std::string> >::const_iterator it = _headers.find(headerName);
	if (it != _headers.end())
	{
		return it->second;
	}
	return std::vector<std::string>();
}

size_t HttpHeaders::getHeadersSize() const
{
	return _rawHeadersSize;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpHeaders::reset()
{
	_headersState = HEADERS_PARSING;
	_headers.clear();
>>>>>>> ConfigParserRefactor
}