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
}