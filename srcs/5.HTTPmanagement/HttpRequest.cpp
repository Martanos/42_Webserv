#include "HttpRequest.hpp"

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
			_parseState = PARSING_HEADERS;
			break;
		case HttpURI::URI_PARSING_ERROR:
			_parseState = PARSING_ERROR;
			break;
		}
		break;
	}
	case PARSING_HEADERS:
	{
		int result = _headers.parseBuffer(_rawBuffer, response);
		if (result == HttpHeaders::HEADERS_PARSING_COMPLETE)
		{
			_parseState = PARSING_BODY;
		}
		else if (result == HttpHeaders::HEADERS_PARSING_ERROR)
		{
			_parseState = PARSING_ERROR;
		}
		break;
	}
	case PARSING_BODY:
	{
		int result = _body.parseBuffer(_rawBuffer, response);
		if (result == HttpBody::BODY_PARSING_COMPLETE)
		{
			_parseState = PARSING_COMPLETE;
		}
		else if (result == PARSING_ERROR)
		{
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
	return _parseState == PARSE_COMPLETE;
}

bool HttpRequest::hasError() const
{
	return _parseState == PARSE_ERROR;
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
	_headers.clear();
	_body.clear();
	_headers.setExpectedBodySize(0);
	_headers.setBodyType(HttpHeaders::BODY_TYPE_NO_BODY);
	_parseState = PARSE_REQUEST_LINE;
	_rawBuffer.clear();
	_bytesReceived = 0;
}

const std::string &HttpRequest::getMethod() const
{
	return _uri.getMethod();
};
const std::string &HttpRequest::getUri() const
{
	return _uri.getURI();
}
const std::string &HttpRequest::getVersion() const
{
	return _uri.getVersion();
};
const std::map<std::string, std::vector<std::string> > &HttpRequest::getHeaders() const
{
	return _headers.getHeaders();
};
const std::string &HttpRequest::getBody() const
{
	return _body.getRawBody();
};
size_t HttpRequest::getContentLength() const
{
	return _headers.getExpectedBodySize();
};
bool HttpRequest::isChunked() const
{
	return _headers.getBodyType() == HttpHeaders::BODY_TYPE_CHUNKED;
};
bool HttpRequest::isUsingTempFile() const
{
	return _body.getIsUsingTempFile();
};
std::string HttpRequest::getTempFile() const
{
	return _body.getTempFilePath();
};
