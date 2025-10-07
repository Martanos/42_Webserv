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
	_uri = HttpURI();
	_headers = HttpHeaders();
	_body = HttpBody();
	_server = NULL;
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
		_server = rhs._server;
	}
	return *this;
}

/*
** --------------------------------- PARSING METHODS
*----------------------------------
*/

HttpRequest::ParseState HttpRequest::parseBuffer(std::vector<char> &holdingBuffer, HttpResponse &response)
{
	PERF_SCOPED_TIMER(http_request_parsing);

	// Delegate to the appropriate parser
	switch (_parseState)
	{
	case PARSING_URI:
	{
		_uri.parseBuffer(holdingBuffer, response);
		switch (_uri.getURIState())
		{
		case HttpURI::URI_PARSING_COMPLETE:
		{
			Logger::debug("HttpRequest: URI parsing complete");
			Logger::debug("URI: " + _uri.getMethod() + " " + _uri.getURI() + " " + _uri.getVersion());
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
		_headers.parseBuffer(holdingBuffer, response, _body);
		switch (_headers.getHeadersState())
		{
		case HttpHeaders::HEADERS_PARSING_COMPLETE:
			Logger::debug("HttpRequest: Headers parsing complete");
			_parseState = PARSING_BODY;
			break;
		case HttpHeaders::HEADERS_PARSING_ERROR:
			Logger::error("HttpRequest: Headers parsing error", __FILE__, __LINE__);
			_parseState = PARSING_ERROR;
			break;
		}
		break;
	}
	case PARSING_BODY:
	{
		if (_server == NULL)
		{
			Logger::error("HttpRequest: No server found", __FILE__, __LINE__);
			response.setStatus(400, "Bad Request");
			_parseState = PARSING_ERROR;
			break;
		}
		_body.parseBuffer(holdingBuffer, response);
		switch (_body.getBodyState())
		{
		case HttpBody::BODY_PARSING_COMPLETE:
		{
			Logger::debug("HttpRequest: Body parsing complete");
			_parseState = PARSING_COMPLETE;
			break;
		}
		case HttpBody::BODY_PARSING:
			_parseState = PARSING_BODY;
			break;
		case HttpBody::BODY_PARSING_ERROR:
			Logger::error("HttpRequest: Body parsing error", __FILE__, __LINE__);
			_parseState = PARSING_ERROR;
			break;
		}
		break;
	}
	default:
		break;
	}
	// TODO: Log state here
	return _parseState;
}

void HttpRequest::sanitizeRequest(HttpResponse &response, const Server *server, const Location *location)
{
	_uri.sanitizeURI(server, location);
	if (_uri.getURIState() == HttpURI::URI_PARSING_ERROR)
	{
		response.setStatus(400, "Bad Request");
		_parseState = PARSING_ERROR;
		return;
	}
}

/*
** --------------------------------- ACCESSOR METHODS
*----------------------------------
*/

const std::vector<std::string> HttpRequest::getHeader(const std::string &name) const
{
	return _headers.getHeader(name);
}

// Enhanced reset method
void HttpRequest::reset()
{
	_headers.reset();
	_body.reset();
	_body.setExpectedBodySize(0);
	_body.setBodyType(HttpBody::BODY_TYPE_NO_BODY);
	_parseState = PARSING_URI;
}

/*
** --------------------------------- MUTATOR METHODS
*----------------------------------
*/

void HttpRequest::setParseState(ParseState parseState)
{
	_parseState = parseState;
}
void HttpRequest::setServer(Server *server)
{
	_server = server;
}

/*
** --------------------------------- ACCESSOR METHODS
*----------------------------------
*/

HttpRequest::ParseState HttpRequest::getParseState() const
{
	return _parseState;
};

// URI accessors
std::string HttpRequest::getMethod() const
{
	return _uri.getMethod();
};

std::string HttpRequest::getUri() const
{
	return _uri.getURI();
};

std::string HttpRequest::getVersion() const
{
	return _uri.getVersion();
};

std::map<std::string, std::vector<std::string> > HttpRequest::getHeaders() const
{
	return _headers.getHeaders();
};

std::string HttpRequest::getBodyData() const
{
	return _body.getRawBody();
};

HttpBody::BodyType HttpRequest::getBodyType() const
{
	return _body.getBodyType();
};

size_t HttpRequest::getMessageSize() const
{
	return _uri.getRawURISize() + _headers.getHeadersSize() + _body.getBodySize();
};

bool HttpRequest::isUsingTempFile()
{
	return _body.getIsUsingTempFile();
};

std::string HttpRequest::getTempFile()
{
	return _body.getTempFilePath();
};

Server *HttpRequest::getServer() const
{
	return _server;
};