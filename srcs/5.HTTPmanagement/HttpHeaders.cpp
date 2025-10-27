#include "../../includes/HTTP/HttpHeaders.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/HTTP/HTTP.hpp"
#include "../../includes/HTTP/HttpBody.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
#include <algorithm>
#include <cctype>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpHeaders::HttpHeaders()
{
	_headersState = HEADERS_PARSING;
	_headers = std::vector<Header>();
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
	std::string rawHeader;
	rawHeader.assign(buffer.begin(), it);
	buffer.erase(buffer.begin(), it + 2);
	if (rawHeader.empty())
	{
		parseAllHeaders(response, body);
		_headersState = HEADERS_PARSING_COMPLETE;
		Logger::log(Logger::DEBUG, "Headers parsing complete");
		return;
	}
	else if (rawHeader.size() + 2 > HTTP::MAX_HEADERS_LINE_SIZE)
	{
		response.setStatus(413, "Request Line Header Too Large");
		Logger::log(Logger::ERROR, "Header line size limit exceeded");
		_headersState = HEADERS_PARSING_ERROR;
		return;
	}
	_rawHeadersSize = rawHeader.size() + 2;
	if (_rawHeadersSize > HTTP::MAX_HEADERS_SIZE)
	{
		response.setStatus(413, "Request headers total size too large");
		Logger::log(Logger::ERROR, "Header total size limit exceeded");
		_headersState = HEADERS_PARSING_ERROR;
		return;
	}

	// Parse headers
	parseHeaderLine(rawHeader, response);
}

void HttpHeaders::parseHeaderLine(const std::string &rawHeader, HttpResponse &response)
{
	try
	{
		Header header(rawHeader);
		for (std::vector<Header>::iterator it = _headers.begin(); it != _headers.end(); ++it)
		{
			if (*it == header)
			{
				if (isSingletonHeader(header.getDirective()))
				{
					Logger::log(Logger::WARNING, "Singleton header " + header.getDirective() + " found multiple times");
					_headersState = HEADERS_PARSING_ERROR;
					response.setStatus(400, "Bad Request");
					return;
				}
				it->merge(header);
				return;
			}
		}
		_headers.push_back(header); // If the header is not found, add it to the list
	}
	catch (const std::exception &e)
	{
		Logger::log(Logger::ERROR, "Error parsing header: " + std::string(e.what()));
		_headersState = HEADERS_PARSING_ERROR;
		response.setStatus(400, "Bad Request");
	}
}

// Checks all headers for special characters
void HttpHeaders::parseAllHeaders(HttpResponse &response, HttpBody &body)
{
	bool hostFound = false;
	for (std::vector<Header>::iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		std::string headerName = it->getDirective();
		std::vector<std::string> headerValues = it->getValues();
		if (headerName == "content-length")
		{
			if (std::find_if(_headers.begin(), _headers.end(), [](const Header &header)
							 { return header.getDirective() == "transfer-encoding"; }) != _headers.end())
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
			if (std::find_if(_headers.begin(), _headers.end(), [](const Header &header)
							 { return header.getDirective() == "content-length"; }) != _headers.end())
			{
				Logger::log(Logger::WARNING, "Content-Length and Transfer-Encoding headers cannot be used together");
				_headersState = HEADERS_PARSING_ERROR;
				response.setStatus(400, "Bad Request");
				return;
			}
			// As per requirements, only chunked is supported
			if (std::find(headerValues.begin(), headerValues.end(), "chunked") != headerValues.end())
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
			if (headerValues.empty())
			{
				Logger::log(Logger::WARNING, "Empty Host header");
				_headersState = HEADERS_PARSING_ERROR;
				response.setStatus(400, "Bad Request");
				return;
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

bool HttpHeaders::isSingletonHeader(const std::string &headerName) const
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

const std::vector<Header> &HttpHeaders::getHeaders() const
{
	return _headers;
}

const Header *HttpHeaders::getHeader(const std::string &headerName) const
{
	for (std::vector<Header>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		if (it->getDirective() == headerName)
		{
			return &*it;
		}
	}
	return NULL;
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