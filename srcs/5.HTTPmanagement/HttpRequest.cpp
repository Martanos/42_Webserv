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
		this->_method = rhs._method;
		this->_uri = rhs._uri;
		this->_version = rhs._version;
		this->_headers = rhs._headers;
		this->_body = rhs._body;
		this->_contentLength = rhs._contentLength;
		this->_isChunked = rhs._isChunked;
		this->_parseState = rhs._parseState;
		this->_rawBuffer = rhs._rawBuffer;
		this->_bodyBytesReceived = rhs._bodyBytesReceived;
	}
	return *this;
}

std::ostream &operator<<(std::ostream &o, HttpRequest const &i)
{
	o << "Method: " << i.getMethod() << "\n"
	  << "URI: " << i.getUri() << "\n"
	  << "Version: " << i.getVersion() << "\n"
	  << "Body: " << i.getBody() << "\n"
	  << "Content Length: " << i.getContentLength() << "\n"
	  << "Is Chunked: " << (i.isChunked() ? "true" : "false") << "\n"
	  << "Is Complete: " << i.isComplete() << "\n"
	  << "Has Error: " << i.hasError() << "\n";
	return o;
}

/*
** --------------------------------- PARSING METHODS ----------------------------------
*/

HttpRequest::ParseState HttpRequest::parseBuffer(const std::string &buffer)
{
	_rawBuffer += buffer;

	while (_parseState != PARSE_COMPLETE && _parseState != PARSE_ERROR)
	{
		switch (_parseState)
		{
		case PARSE_REQUEST_LINE:
		{
			size_t newlinepos = _rawBuffer.find("\r\n");
			if (newlinepos != std::string::npos)
				break;
			std::string newLine = _rawBuffer.substr(0, newlinepos);
			_rawBuffer.erase(0, newlinepos + 2);
			_parseState = _parseRequestLine(newLine);
			break;
		}
		case PARSE_HEADERS:
		{
			size_t newlinepos = _rawBuffer.find("\r\n");
			if (newlinepos != std::string::npos)
				break;
			std::string newLine = _rawBuffer.substr(0, newlinepos);
			_rawBuffer.erase(0, newlinepos + 2);
			if (newLine.empty())
			{
				if (getHeader("content-length") == "")
				{
					_contentLength = strtol(newLine.c_str(), NULL, 10);
					_parseState = PARSE_BODY;
				}
				else if (getHeader("transfer-encoding") == "chunked")
				{
					_isChunked = true;
					_parseState = PARSE_BODY;
				}
				else if (getHeader("transfer-encoding") != "chunked")
				{
					_parseState = PARSE_ERROR;
					Logger::log(Logger::ERROR, "Invalid header line: " + newLine);
				}
				else
					_parseState = PARSE_COMPLETE;
			}
			_parseState = _parseHeaderLine(newLine);
		}
		case PARSE_BODY:
		{
			_parseState = _parseBody();
			break;
		}
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
		return PARSE_ERROR;
	}

	if (!_isValidMethod(method))
	{
		Logger::log(Logger::ERROR, "Invalid HTTP method: " + method);
		return PARSE_ERROR;
	}

	_method = method;
	_uri = uri;
	_version = version;

	return PARSE_HEADERS;
}

HttpRequest::ParseState HttpRequest::_parseHeaderLine(const std::string &line)
{
	size_t colonPos = line.find(':');
	if (colonPos == std::string::npos)
	{
		Logger::log(Logger::ERROR, "Invalid header line: " + line);
		return PARSE_ERROR;
	}

	std::string name = line.substr(0, colonPos);
	std::string value = line.substr(colonPos + 1);

	// Trim whitespace
	size_t start = value.find_first_not_of(" \t");
	if (start != std::string::npos)
		value = value.substr(start);

	size_t end = value.find_last_not_of(" \t\r\n");
	if (end != std::string::npos)
		value = value.substr(0, end + 1);

	_headers[_toLowerCase(name)] = value;

	return PARSE_HEADERS;
}

HttpRequest::ParseState HttpRequest::_parseBody()
{
	if (_isChunked)
	{
		// TODO: Implement chunked parsing
		_body += _rawBuffer;
		_rawBuffer.clear();
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
		this->_method = rhs._method;
		this->_uri = rhs._uri;
		this->_version = rhs._version;
		this->_headers = rhs._headers;
		this->_body = rhs._body;
		this->_contentLength = rhs._contentLength;
		this->_isChunked = rhs._isChunked;
		this->_parseState = rhs._parseState;
		this->_rawBuffer = rhs._rawBuffer;
		this->_bodyBytesReceived = rhs._bodyBytesReceived;
	}
	return *this;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

bool HttpRequest::_isValidMethod(const std::string &method) const
{
	return (method == "GET" || method == "POST" || method == "DELETE" ||
			method == "HEAD" || method == "PUT" || method == "OPTIONS");
}

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

const std::string &HttpRequest::getMethod() const
{
	return _method;
}

const std::string &HttpRequest::getUri() const
{
	return _uri;
}

const std::string &HttpRequest::getVersion() const
{
	return _version;
}

const std::map<std::string, std::string> &HttpRequest::getHeaders() const
{
	return _headers;
}

const std::string &HttpRequest::getBody() const
{
	return _body;
}

size_t HttpRequest::getContentLength() const
{
	return _contentLength;
}

bool HttpRequest::isChunked() const
{
	return _isChunked;
}

/* ************************************************************************** */
