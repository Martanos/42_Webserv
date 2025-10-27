<<<<<<< HEAD
#include "../../includes/HttpRequest.hpp"
#include "../../includes/PerformanceMonitor.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpRequest::HttpRequest()
{
	_uri = HttpURI();
	_headers = HttpHeaders();
	_body = HttpBody();
	_rawBuffer = RingBuffer(sysconf(_SC_PAGESIZE) * 4); // Should equal 32KB/64KB depending on the system
	_bytesReceived = 0;
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
		_rawBuffer = rhs._rawBuffer;
		_bytesReceived = rhs._bytesReceived;
	}
	return *this;
}

/*
** --------------------------------- PARSING METHODS
*----------------------------------
*/

HttpRequest::ParseState HttpRequest::parseBuffer(RingBuffer &buffer, HttpResponse &response)
{
	PERF_SCOPED_TIMER(http_request_parsing);
	
	// Max body size in done in client
	// Flush client buffer into request buffer
	_bytesReceived += buffer.readable();
	_rawBuffer.transferFrom(buffer, buffer.readable());
	switch (_parseState)
	{
	case PARSING_REQUEST_LINE:
	{
		int result = _uri.parseBuffer(_rawBuffer, response);
		switch (result)
		{
		case HttpURI::URI_PARSING_COMPLETE:
			Logger::debug("HttpRequest: Request line parsing complete");
			_parseState = PARSING_HEADERS;
			break;
		case HttpURI::URI_PARSING_ERROR:
			Logger::error("HttpRequest: Request line parsing error");
			_parseState = PARSING_ERROR;
			break;
		}
		break;
	}
	case PARSING_HEADERS:
	{
		int result = _headers.parseBuffer(_rawBuffer, response, _body);
		if (result == HttpHeaders::HEADERS_PARSING_COMPLETE)
		{
			Logger::debug("HttpRequest: Headers parsing complete");
			_parseState = PARSING_BODY;
		}
		else if (result == HttpHeaders::HEADERS_PARSING_ERROR)
		{
			Logger::error("HttpRequest: Headers parsing error");
			_parseState = PARSING_ERROR;
		}
		break;
	}
	case PARSING_BODY:
	{
		int result = _body.parseBuffer(_rawBuffer, response);
		if (result == HttpBody::BODY_PARSING_COMPLETE)
		{
			Logger::debug("HttpRequest: Body parsing complete");
			_parseState = PARSING_COMPLETE;
		}
		else if (result == PARSING_ERROR)
		{
			Logger::error("HttpRequest: Body parsing error", __FILE__, __LINE__);
			_parseState = PARSING_ERROR;
		}
		break;
	}
	default:
		break;
	}
	return _parseState;
}

/*
** --------------------------------- ACCESSOR METHODS
*----------------------------------
*/

bool HttpRequest::isComplete() const
{
	return _parseState == PARSING_COMPLETE;
}

bool HttpRequest::hasError() const
{
	return _parseState == PARSING_ERROR;
}

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
	_headers.setExpectedBodySize(0);
	_body.setBodyType(HttpBody::BODY_TYPE_NO_BODY);
	_parseState = PARSING_REQUEST_LINE;
	_rawBuffer.clear();
	_bytesReceived = 0;
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
bool HttpRequest::isChunked() const
{
	return _body.getIsChunked();
};
bool HttpRequest::isUsingTempFile() const
{
	return _body.getIsUsingTempFile();
};
std::string HttpRequest::getTempFile() const
{
	return _body.getTempFilePath();
};
=======
#include "../../includes/HTTP/HttpRequest.hpp"
#include "../../includes/Core/Server.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/PerformanceMonitor.hpp"
#include "../../includes/HTTP/HttpBody.hpp"
#include "../../includes/HTTP/HttpHeaders.hpp"
#include "../../includes/HTTP/HttpURI.hpp"

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
			Logger::error("HttpRequest: Headers parsing error", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			_parseState = PARSING_ERROR;
			break;
		}
		break;
	}
	case PARSING_BODY:
	{
		if (_server == NULL)
		{
			Logger::error("HttpRequest: No server found", __FILE__, __LINE__, __PRETTY_FUNCTION__);
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
			Logger::error("HttpRequest: Body parsing error", __FILE__, __LINE__, __PRETTY_FUNCTION__);
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

// TODO: change to reference
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
	return _uri.getURIsize() + _headers.getHeadersSize() + _body.getBodySize();
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
>>>>>>> ConfigParserRefactor
