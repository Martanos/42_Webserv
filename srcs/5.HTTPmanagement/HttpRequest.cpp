#include "HttpRequest.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpRequest::HttpRequest()
{
	_method = "";
	_uri = "";
	_version = "";
	_headers = std::map<std::string, std::string>();
	_body = "";
	_contentLength = 0;
	_isChunked = false;
	_parseState = PARSE_REQUEST_LINE;
	_rawBuffer = "";
	_bodyBytesReceived = 0;
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
	_headers.clear();
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

HttpRequest &HttpRequest::operator=(HttpRequest const &rhs)
{
	if (this != &rhs)
	{
		_method = rhs._method;
		_uri = rhs._uri;
		_version = rhs._version;
		_headers = rhs._headers;
		_body = rhs._body;
		_contentLength = rhs._contentLength;
		_isChunked = rhs._isChunked;
		_parseState = rhs._parseState;
		_rawBuffer = rhs._rawBuffer;
		_bodyBytesReceived = rhs._bodyBytesReceived;
	}
	return *this;
}

/*
** --------------------------------- PARSING METHODS ----------------------------------
*/

HttpRequest::ParseState HttpRequest::parseBuffer(const std::string &buffer, size_t bodyBufferSize)
{
	_rawBuffer += buffer;

	while (_parseState != PARSE_COMPLETE && _parseState != PARSE_ERROR)
	{
		switch (_parseState)
		{
		case PARSE_REQUEST_LINE:
		{
			if (_rawBuffer.empty())
			{
				return _parseState;
			}
			else if (_rawBuffer.size() > sysconf(_SC_PAGE_SIZE))
			{
				return PARSE_PAYLOAD_TOO_LARGE_REQUEST_LINE;
			}
			size_t newlinePos = _rawBuffer.find("\r\n");
			if (newlinePos == std::string::npos)
			{
				return _parseState;
			}
			std::string requestLine = _rawBuffer.substr(0, newlinePos);
			_rawBuffer.erase(0, newlinePos + 2);
			_parseState = _parseRequestLine(requestLine);
			break;
		}
		case PARSE_HEADERS:
		{
			if (_rawBuffer.empty())
			{
				return _parseState;
			}
			else if (_rawBuffer.size() > bodyBufferSize)
			{
				return PARSE_PAYLOAD_TOO_LARGE_HEADERS;
			}
			size_t newlinePos = _rawBuffer.find("\r\n");
			if (newlinePos == std::string::npos)
			{
				return _parseState;
			}
			std::string headerLine = _rawBuffer.substr(0, newlinePos);
			_rawBuffer.erase(0, newlinePos + 2);

			if (headerLine.empty())
			{
				// Empty line indicates end of headers
				_prepareForBody();
				if (_contentLength == 0 && !_isChunked)
				{
					_parseState = PARSE_COMPLETE;
				}
				else
				{
					_parseState = PARSE_BODY;
				}
			}
			else
			{
				_parseState = _parseHeaderLine(headerLine);
			}
			break;
		}
		case PARSE_BODY:
		{
			if (_rawBuffer.size() > bodyBufferSize)
			{
				return PARSE_PAYLOAD_TOO_LARGE_BODY;
			}
			_parseState = _parseBody();
			break;
		}
		case PARSE_COMPLETE:
			break;
		case PARSE_ERROR:
			break;
		}
	}
	return _parseState;
}

HttpRequest::ParseState HttpRequest::_parseRequestLine(const std::string &line)
{
	std::istringstream iss(line);
	std::string method, uri, version;

	if (!(iss >> method >> uri >> version))
	{
		Logger::log(Logger::ERROR, "Invalid request line: " + line);
		return PARSE_ERROR_INVALID_REQUEST_LINE;
	}

	if (!_isValidMethod(method))
	{
		Logger::log(Logger::ERROR, "Invalid HTTP method: " + method);
		return PARSE_ERROR_INVALID_METHOD;
	}

	_method = method;
	_uri = uri;
	_version = version;

	return PARSE_HEADERS;
}

HttpRequest::ParseState HttpRequest::_parseHeaderLine(const std::string &line)
{
	// Extract the header name and value
	size_t colonPos = line.find(':');
	if (colonPos == std::string::npos)
	{
		Logger::log(Logger::ERROR, "Invalid header line: " + line);
		return PARSE_ERROR;
	}

	std::string name = line.substr(0, colonPos);
	std::string value = line.substr(colonPos + 1);

	// Trim whitespace from value
	size_t start = value.find_first_not_of(" \t");
	if (start != std::string::npos)
		value = value.substr(start);

	size_t end = value.find_last_not_of(" \t\r\n");
	if (end != std::string::npos)
		value = value.substr(0, end + 1);

	// Attempt to insert lowercase header name into headers map
	if (_headers.insert(std::make_pair(_toLowerCase(name), value)).second)
	{
		return PARSE_ERROR_INTERNAL;
	}

	return PARSE_HEADERS;
}

void HttpRequest::_prepareForBody()
{
	std::map<std::string, std::string>::const_iterator contentLengthIt = _headers.find("content-length");
	std::map<std::string, std::string>::const_iterator transferEncodingIt = _headers.find("transfer-encoding");

	if (contentLengthIt != _headers.end())
	{
		_contentLength = static_cast<size_t>(std::strtoul(contentLengthIt->second.c_str(), NULL, 10));
	}
	else if (transferEncodingIt != _headers.end() && transferEncodingIt->second == "chunked")
	{
		_isChunked = true;
	}
}

HttpRequest::ParseState HttpRequest::_parseBody()
{
	if (_isChunked)
	{
		// Simple chunked parsing - for full implementation, need to handle chunk sizes
		// This is a simplified version that just collects all data
		_body += _rawBuffer;
		_rawBuffer.clear();
		// TODO: Implement proper chunked parsing
		return PARSE_COMPLETE;
	}
	else if (_contentLength > 0)
	{
		size_t needed = _contentLength - _bodyBytesReceived;
		size_t available = _rawBuffer.length();
		size_t toRead = (needed < available) ? needed : available;

		_body += _rawBuffer.substr(0, toRead);
		_rawBuffer.erase(0, toRead);
		_bodyBytesReceived += toRead;

		if (_bodyBytesReceived >= _contentLength)
		{
			return PARSE_COMPLETE;
		}
	}

	return PARSE_BODY;
}

std::string HttpRequest::_toLowerCase(const std::string &str) const
{
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

bool HttpRequest::_isValidMethod(const std::string &method) const
{
	return (method == "GET" || method == "POST" || method == "DELETE" ||
			method == "HEAD" || method == "PUT" || method == "OPTIONS");
}

/*
** --------------------------------- ACCESSOR METHODS ----------------------------------
*/

bool HttpRequest::isComplete() const
{
	return _parseState == PARSE_COMPLETE;
}

bool HttpRequest::hasError() const
{
	return _parseState == PARSE_ERROR;
}

const std::string &HttpRequest::getHeader(const std::string &name) const
{
	static const std::string empty = "";
	std::map<std::string, std::string>::const_iterator it = _headers.find(_toLowerCase(name));
	return (it != _headers.end()) ? it->second : empty;
}

void HttpRequest::reset()
{
	_method.clear();
	_uri.clear();
	_version.clear();
	_headers.clear();
	_body.clear();
	_contentLength = 0;
	_isChunked = false;
	_parseState = PARSE_REQUEST_LINE;
	_rawBuffer.clear();
	_bodyBytesReceived = 0;
}

const std::string &HttpRequest::getMethod() const { return _method; };
const std::string &HttpRequest::getUri() const { return _uri; }
const std::string &HttpRequest::getVersion() const { return _version; };
const std::map<std::string, std::string> &HttpRequest::getHeaders() const { return _headers; };
const std::string &HttpRequest::getBody() const { return _body; };
size_t HttpRequest::getContentLength() const { return _contentLength; };
bool HttpRequest::isChunked() const { return _isChunked; };
