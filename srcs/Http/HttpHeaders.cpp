#include "../../includes/Http/HttpHeaders.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Http/HTTP.hpp"
#include "../../includes/Http/HttpBody.hpp"
#include "../../includes/Http/HttpResponse.hpp"
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
	Logger::debug("HttpHeaders: Parsing buffer, size: " + StrUtils::toString(buffer.size()), __FILE__, __LINE__,
				  __PRETTY_FUNCTION__);
	std::string bufferStr(buffer.begin(), buffer.end());
	Logger::debug("HttpHeaders: Buffer content: " + bufferStr, __FILE__, __LINE__, __PRETTY_FUNCTION__);

	// Continue parsing headers until we find empty line or run out of data
	while (_headersState == HEADERS_PARSING && !buffer.empty())
	{
		std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), HTTP::CRLF, HTTP::CRLF + 2);
		if (it == buffer.end())
		{
			Logger::debug("HttpHeaders: No CRLF found, waiting for more data", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			// If it can't be found check that the buffer has not currently exceeded the size limit of a header
			if (buffer.size() > HTTP::DEFAULT_CLIENT_MAX_HEADERS_SIZE)
			{
				response.setResponseDefaultBody(413, "Request Header Too Large", NULL, NULL, HttpResponse::FATAL_ERROR);
				Logger::debug("Header size limit exceeded", __FILE__, __LINE__, __PRETTY_FUNCTION__);
				_headersState = HEADERS_PARSING_ERROR;
			}
			else
				_headersState = HEADERS_PARSING;
			return;
		}

		// Extract header data from buffer
		std::string rawHeader;
		rawHeader.assign(buffer.begin(), it);
		Logger::debug("HttpHeaders: Found header line: '" + rawHeader + "'", __FILE__, __LINE__, __PRETTY_FUNCTION__);
		buffer.erase(buffer.begin(), it + 2);
		if (rawHeader.empty())
		{
			Logger::debug("HttpHeaders: Empty line found, headers complete", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			parseAllHeaders(response, body);
			_headersState = HEADERS_PARSING_COMPLETE;
			Logger::log(Logger::DEBUG, "Headers parsing complete");
			return;
		}
		else if (rawHeader.size() + 2 > HTTP::DEFAULT_CLIENT_MAX_HEADERS_SIZE)
		{
			response.setResponseDefaultBody(413, "Request Line Header Too Large", NULL, NULL,
											HttpResponse::FATAL_ERROR);
			Logger::debug("Header line size limit exceeded", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			_headersState = HEADERS_PARSING_ERROR;
			return;
		}
		_rawHeadersSize += rawHeader.size() + 2;
		if (_rawHeadersSize > HTTP::DEFAULT_CLIENT_MAX_HEADERS_SIZE)
		{
			response.setResponseDefaultBody(413, "Request headers total size too large", NULL, NULL,
											HttpResponse::FATAL_ERROR);
			Logger::debug("Header total size limit exceeded", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			_headersState = HEADERS_PARSING_ERROR;
			return;
		}

		// Parse headers
		parseHeaderLine(rawHeader, response);
		if (_headersState == HEADERS_PARSING_ERROR)
		{
			return;
		}
	}
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
					Logger::debug("Singleton header " + header.getDirective() + " found multiple times", __FILE__,
								  __LINE__, __PRETTY_FUNCTION__);
					_headersState = HEADERS_PARSING_ERROR;
					response.setResponseDefaultBody(400, "Duplicate singleton header found", NULL, NULL,
													HttpResponse::FATAL_ERROR);
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
		response.setResponseDefaultBody(400, "Error parsing header: " + std::string(e.what()), NULL, NULL,
										HttpResponse::FATAL_ERROR);
		_headersState = HEADERS_PARSING_ERROR;
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
			bool hasTransferEncoding = false;
			for (std::vector<Header>::const_iterator headerIt = _headers.begin(); headerIt != _headers.end();
				 ++headerIt)
			{
				if (headerIt->getDirective() == "transfer-encoding")
				{
					hasTransferEncoding = true;
					break;
				}
			}
			if (hasTransferEncoding)
			{
				Logger::debug("Content-Length and Transfer-Encoding headers cannot be used together", __FILE__,
							  __LINE__, __PRETTY_FUNCTION__);
				_headersState = HEADERS_PARSING_ERROR;
				response.setResponseDefaultBody(400,
												"Content-Length and Transfer-Encoding headers cannot be used together",
												NULL, NULL, HttpResponse::FATAL_ERROR);
				return;
			}
			char *endPtr;
			ssize_t contentLength = std::strtol(headerValues[0].c_str(), &endPtr, 10);
			if (*endPtr != '\0' || contentLength < 0)
			{
				Logger::debug("Invalid Content-Length header: " + headerValues[0], __FILE__, __LINE__,
							  __PRETTY_FUNCTION__);
				_headersState = HEADERS_PARSING_ERROR;
				response.setResponseDefaultBody(400, "Invalid Content-Length header: " + headerValues[0], NULL, NULL,
												HttpResponse::FATAL_ERROR);
				return;
			}
			if (contentLength > 0)
			{
				body.setExpectedBodySize(contentLength);
				body.setBodyType(HttpBody::BODY_TYPE_CONTENT_LENGTH);
			}
			else
			{
				body.setBodyType(HttpBody::BODY_TYPE_NO_BODY);
			}
		}
		else if (headerName == "transfer-encoding")
		{
			bool hasContentLength = false;
			for (std::vector<Header>::const_iterator headerIt = _headers.begin(); headerIt != _headers.end();
				 ++headerIt)
			{
				if (headerIt->getDirective() == "content-length")
				{
					hasContentLength = true;
					break;
				}
			}
			if (hasContentLength)
			{
				Logger::debug("Content-Length and Transfer-Encoding headers cannot be used together", __FILE__,
							  __LINE__, __PRETTY_FUNCTION__);
				_headersState = HEADERS_PARSING_ERROR;
				response.setResponseDefaultBody(400,
												"Content-Length and Transfer-Encoding headers cannot be used together",
												NULL, NULL, HttpResponse::FATAL_ERROR);
				return;
			}
			// As per requirements, only chunked is supported
			if (std::find(headerValues.begin(), headerValues.end(), "chunked") != headerValues.end())
			{
				body.setBodyType(HttpBody::BODY_TYPE_CHUNKED);
			}
			else
			{
				Logger::debug("Invalid Transfer-Encoding header: " + headerValues[0], __FILE__, __LINE__,
							  __PRETTY_FUNCTION__);
				_headersState = HEADERS_PARSING_ERROR;
				response.setResponseDefaultBody(400, "Invalid Transfer-Encoding header: " + headerValues[0], NULL, NULL,
												HttpResponse::FATAL_ERROR);
				return;
			}
		}
		else if (headerName == "connection")
		{
			if (headerValues[0] == "close")
			{
				response.setHeader(Header("Connection: close"));
			}
			else if (headerValues[0] == "keep-alive")
			{
				response.setHeader(Header("Connection: keep-alive"));
			}
			else
			{
				Logger::debug("Invalid Connection header: " + headerValues[0], __FILE__, __LINE__, __PRETTY_FUNCTION__);
				_headersState = HEADERS_PARSING_ERROR;
				response.setResponseDefaultBody(400, "Invalid Connection header: " + headerValues[0], NULL, NULL,
												HttpResponse::FATAL_ERROR);
				return;
			}
		}
		else if (headerName == "host")
		{
			hostFound = true;
			// Host header is required for HTTP/1.1
			if (headerValues.empty())
			{
				Logger::debug("Empty Host header", __FILE__, __LINE__, __PRETTY_FUNCTION__);
				_headersState = HEADERS_PARSING_ERROR;
				response.setResponseDefaultBody(400, "Empty Host header", NULL, NULL, HttpResponse::FATAL_ERROR);
				return;
			}
		}
	}
	if (!hostFound)
	{
		Logger::debug("Host header is required for this server", __FILE__, __LINE__, __PRETTY_FUNCTION__);
		_headersState = HEADERS_PARSING_ERROR;
		response.setResponseDefaultBody(400, "Host header is required for this server", NULL, NULL,
										HttpResponse::FATAL_ERROR);
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
