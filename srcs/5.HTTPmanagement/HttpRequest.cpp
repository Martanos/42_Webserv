#include "../../includes/HttpRequest.hpp"
#include "../../includes/PerformanceMonitor.hpp"

#include "Constants.hpp"
#include "FileDescriptor.hpp"
#include "HttpBody.hpp"
#include "HttpHeaders.hpp"
#include "HttpURI.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "StringUtils.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpRequest::HttpRequest()
{
	_parseState = PARSING_URI;
	_rawBuffer = std::vector<char>();
	_totalBytesReceived = 0;
	_uri = HttpURI();
	_headers = HttpHeaders();
	_body = HttpBody();
}

HttpRequest::HttpRequest(const HttpRequest &src)
{
	*this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

HttpRequest::~HttpRequest()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

HttpRequest &HttpRequest::operator=(const HttpRequest &rhs)
{
	if (this != &rhs)
	{
		_uri = rhs._uri;
		_headers = rhs._headers;
		_body = rhs._body;
		_parseState = rhs._parseState;
	}
	return *this;
}

/*
** --------------------------------- PARSING METHODS
*----------------------------------
*/

HttpRequest::ParseState HttpRequest::parseBuffer(std::vector<char> &buffer, HttpResponse &response, Server *server,
												 size_t bytesRead)
{
	PERF_SCOPED_TIMER(http_request_parsing);

	// Ingest buffer into a vector first helps to prevent data corruption
	_rawBuffer.insert(_rawBuffer.end(), buffer.begin(), buffer.end());
	_totalBytesReceived += bytesRead;

	if (server != NULL)
	{
		if (server->getClientMaxBodySize() < _totalBytesReceived)
		{
			Logger::error("HttpRequest: Request body too large");
			_parseState = PARSING_ERROR;
			return _parseState;
		}
	}

	while (_rawBuffer.size() > 0)
	{
		// Delegate to the appropriate parser
		switch (_parseState)
		{
		case PARSING_URI:
		{
			_uri.parseBuffer(_rawBuffer, response);
			switch (_uri.getURIState())
			{
			case HttpURI::URI_PARSING_COMPLETE:
			{
				Logger::debug("HttpRequest: URI parsing complete");
				Logger::debug("URI: " + _uri.getMethod() + " " + _uri.getURI() + " " + _uri.getVersion());
				printf("HttpRequest::parseBuffer after URI parsing\n");
				printf("rawBuffer size: %zu\n", _rawBuffer.size());
				for (std::vector<char>::iterator it = _rawBuffer.begin(); it != _rawBuffer.end(); it++)
				{
					printf("%c", *it);
				}
				printf("\n");
				_parseState = PARSING_HEADERS;
				break;
			}
			case HttpURI::URI_PARSING_ERROR:
				Logger::error("HttpRequest: Request line parsing error");
				_parseState = PARSING_ERROR;
				break;
			default:
				break;
			}
			break;
		}
		case PARSING_HEADERS:
		{
			_headers.parseBuffer(_rawBuffer, response, _body);
			if (_headers.getHeadersState() == HttpHeaders::HEADERS_PARSING_COMPLETE)
			{
				Logger::debug("HttpRequest: Headers parsing complete");
				_parseState = PARSING_BODY;
			}
			else if (_headers.getHeadersState() == HttpHeaders::HEADERS_PARSING_ERROR)
			{
				Logger::error("HttpRequest: Headers parsing error");
				_parseState = PARSING_ERROR;
			}
			break;
		}
		case PARSING_BODY:
		{
			_body.parseBuffer(_rawBuffer, response);
			if (_body.getBodyState() == HttpBody::BODY_PARSING_COMPLETE)
			{
				Logger::debug("HttpRequest: Body parsing complete");
				_parseState = PARSING_COMPLETE;
			}
			else if (_body.getBodyState() == HttpBody::BODY_PARSING_ERROR)
			{
				Logger::error("HttpRequest: Body parsing error", __FILE__, __LINE__);
				_parseState = PARSING_ERROR;
			}
			break;
		}
		default:
			break;
		}
		// TODO: Log state here
	}
	return _parseState;
}

/*
** --------------------------------- ACCESSOR METHODS
*----------------------------------
*/

const std::vector<std::string> &HttpRequest::getHeader(const std::string &name) const
{
	static const std::vector<std::string> empty = std::vector<std::string>();
	std::map<std::string, std::vector<std::string> >::const_iterator it =
		_headers.getHeaders().find(StringUtils::toLowerCase(name));
	return it != _headers.getHeaders().end() ? it->second : empty;
}

// Enhanced reset method
void HttpRequest::reset()
{
	_headers.reset();
	_body.reset();
	_body.setExpectedBodySize(0);
	_body.setBodyType(HttpBody::BODY_TYPE_NO_BODY);
	_parseState = PARSING_URI;
	_rawBuffer.clear();
	_totalBytesReceived = 0;
}

std::string HttpRequest::getMethod() const
{
	return _uri.getMethod();
};
std::string HttpRequest::getUri() const
{
	return _uri.getURI();
}
std::string HttpRequest::getVersion() const
{
	return _uri.getVersion();
};
std::map<std::string, std::vector<std::string> > HttpRequest::getHeaders() const
{
	return _headers.getHeaders();
};
std::string HttpRequest::getBody() const
{
	return _body.getRawBody();
};
size_t HttpRequest::getContentLength() const
{
	return _headers.getExpectedBodySize();
};
bool HttpRequest::isChunked()
{
	return _body.getIsChunked();
};
bool HttpRequest::isUsingTempFile()
{
	return _body.getIsUsingTempFile();
};
std::string HttpRequest::getTempFile()
{
	return _body.getTempFilePath();
};
