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

    size_t newBytes = buffer.readable();
    _bytesReceived += newBytes;

    std::string temp;
    buffer.peekBuffer(temp, newBytes);
    _rawBuffer.writeBuffer(temp.c_str(), temp.size());
    Logger::debug("HttpRequest: Moved " + StringUtils::toString(temp.size()) + 
                  " bytes into _rawBuffer; now has " + StringUtils::toString(_rawBuffer.readable()));

    bool progressed = true;
    while (progressed)
    {
        progressed = false;

        switch (_parseState)
        {
        case PARSING_REQUEST_LINE:
        {
            int result = _uri.parseBuffer(_rawBuffer, response);
            if (result == HttpURI::URI_PARSING_COMPLETE)
            {
                Logger::debug("HttpRequest: Request line parsing complete");
                _parseState = PARSING_HEADERS;
                progressed = true; // continue immediately to headers
            }
            else if (result == HttpURI::URI_PARSING_ERROR)
            {
                _parseState = PARSING_ERROR;
            }
            break;
        }

        case PARSING_HEADERS:
        {
            int result = _headers.parseBuffer(_rawBuffer, response, _body);
            Logger::debug("HttpRequest: Header parseBuffer() returned = " +
                          StringUtils::toString(result));
            if (result == HttpHeaders::HEADERS_PARSING_COMPLETE)
            {
                Logger::debug("HttpRequest: Headers parsing complete");
                _parseState = PARSING_BODY;
                progressed = true;
            }
            else if (result == HttpHeaders::HEADERS_PARSING_ERROR)
            {
                _parseState = PARSING_ERROR;
            }
            else
            {
                Logger::debug("HttpRequest: Still parsing headers... waiting for more data");
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
    }

    Logger::debug("HttpRequest::parseBuffer finished with state = " +
                  StringUtils::toString(_parseState));

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
