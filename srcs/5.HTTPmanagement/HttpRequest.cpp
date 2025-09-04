#include "HttpRequest.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpRequest::HttpRequest()
{
	_tempFd = FileDescriptor(-1);
	_method = "";
	_uri = "";
	_version = "";
	_headers = std::map<std::string, std::vector<std::string> >();
	_body = "";
	_contentLength = 0;
	_isChunked = false;
	_parseState = PARSE_REQUEST_LINE;
	_rawBuffer = "";
	_bytesReceived = 0;
	_maxHeaderSize = HTTP::DEFAULT_BUFFER_SIZE;
	_maxBodySize = HTTP::DEFAULT_BUFFER_SIZE;
	_maxRequestLineSize = HTTP::DEFAULT_BUFFER_SIZE;
	_maxContentLength = HTTP::DEFAULT_MAX_CONTENT_LENGTH;
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
		_bytesReceived = rhs._bytesReceived;
	}
	return *this;
}

/*
** --------------------------------- PARSING METHODS ----------------------------------
*/

HttpRequest::ParseState HttpRequest::parseBuffer(const std::string &buffer, ssize_t bodyBufferSize)
{
	_rawBuffer += buffer;
	_bytesReceived += buffer.size();

	if (_bytesReceived > _maxContentLength)
	{
		return PARSE_ERROR_CONTENT_LENGTH_TOO_LONG;
	}
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
			else if (_rawBuffer.size() > _maxRequestLineSize)
			{
				return PARSE_ERROR_REQUEST_LINE_TOO_LONG;
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
			else if (_rawBuffer.size() > _maxHeaderSize)
			{
				return PARSE_ERROR_HEADER_TOO_LONG;
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
				_parseState = _parseHeaders();
				if (_parseState != PARSE_HEADERS)
				{
					return _parseState;
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
				return PARSE_ERROR;
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
	std::string extraInfo;

	if (!(iss >> method >> uri >> version))
	{
		Logger::log(Logger::ERROR, "Invalid request line: " + line);
		return PARSE_ERROR_INVALID_REQUEST_LINE;
	}
	else if (method.empty() || uri.empty() || version.empty())
	{
		Logger::log(Logger::ERROR, "Invalid request line: " + line);
		return PARSE_ERROR_INVALID_REQUEST_LINE;
	}
	else if (version != "HTTP/1.1")
	{
		Logger::log(Logger::ERROR, "Invalid HTTP version: " + version);
		return PARSE_ERROR_INVALID_HTTP_VERSION;
	}
	else if (!iss.eof())
	{
		iss >> extraInfo;
		Logger::log(Logger::ERROR, "Invalid request line: " + line + " - Extra info: " + extraInfo);
		return PARSE_ERROR_MALFORMED_REQUEST;
	}
	else if (!_isValidMethod(method))
	{
		Logger::log(Logger::ERROR, "Invalid HTTP method: " + method);
		return PARSE_ERROR_INVALID_HTTP_METHOD;
	}

	_method = method;
	_uri = uri;
	_version = version;

	return PARSE_HEADERS;
}

HttpRequest::ParseState HttpRequest::_parseHeaderLine(const std::string &line)
{
	// Extract the header and attempt to insert into headers map
	std::stringstream ss(line);
	std::string name, token;

	if (!(ss >> name))
	{
		Logger::log(Logger::ERROR, "Internal server error: " + line);
		return PARSE_ERROR_INTERNAL_SERVER_ERROR;
	}
	ss >> name;
	size_t colonPos = name.find(':');
	if (colonPos == std::string::npos)
	{
		Logger::log(Logger::ERROR, "Invalid header line: " + line);
		return PARSE_ERROR_MALFORMED_REQUEST;
	}
	std::string name = line.substr(0, colonPos);

	// Find if header already exists, if it doesnt create new key with empty vector
	std::map<std::string, std::vector<std::string> >::iterator it = _headers.find(_toLowerCase(name));
	if (it == _headers.end())
	{
		it = _headers.insert(std::make_pair(_toLowerCase(name), std::vector<std::string>())).first;
		if (it == _headers.end())
		{
			Logger::log(Logger::ERROR, "Internal server error: " + line);
			return PARSE_ERROR_INTERNAL_SERVER_ERROR;
		}
	}
	// Insert whitespace/comma separated values into vector
	while (std::getline(ss, token, ','))
	{
		size_t start = 0;
		while (start < token.size() && std::isspace(static_cast<unsigned char>(token[start])))
			++start;
		if (start == token.size())
		{
			token = "";
			it->second.push_back(token);
			continue;
		}
		size_t end = token.size() - 1;
		while (end > start && std::isspace(static_cast<unsigned char>(token[end])))
			--end;
		if (start != std::string::npos && end != std::string::npos)
			token = token.substr(start, end - start + 1);
		else
			token = "";

		it->second.push_back(token);
	}

	return PARSE_HEADERS;
}

HttpRequest::ParseState HttpRequest::_parseHeaders()
{
	// Verify crucial headers first
	// Host header
	std::map<std::string, std::vector<std::string> >::const_iterator hostIt = _headers.find("host");
	if (hostIt == _headers.end())
	{
		Logger::log(Logger::ERROR, "Host header is missing");
		return PARSE_ERROR_MALFORMED_REQUEST;
	}
	else if (hostIt->second.empty())
	{
		Logger::log(Logger::ERROR, "Host header is empty");
		return PARSE_ERROR_MALFORMED_REQUEST;
	}
	else if (hostIt->second.size() > 1)
	{
		Logger::log(Logger::ERROR, "Multiple differing definitions of host header found");
		return PARSE_ERROR_MALFORMED_REQUEST;
	}

	// Connection header
	std::map<std::string, std::vector<std::string> >::const_iterator connectionIt = _headers.find("connection");
	if (connectionIt == _headers.end())
	{
		Logger::log(Logger::ERROR, "Connection header is missing");
		return PARSE_ERROR_MALFORMED_REQUEST;
	}
	else if (connectionIt->second.empty())
	{
		Logger::log(Logger::ERROR, "Connection header is empty");
		return PARSE_ERROR_MALFORMED_REQUEST;
	}
	else if (connectionIt->second.size() > 1)
	{
		Logger::log(Logger::ERROR, "Multiple differing definitions of connection header found");
		return PARSE_ERROR_MALFORMED_REQUEST;
	}
	else if (connectionIt->second[0] != "keep-alive" && connectionIt->second[0] != "close")
	{
		Logger::log(Logger::ERROR, "Invalid connection type: " + connectionIt->second[0]);
		return PARSE_ERROR_MALFORMED_REQUEST;
	}

	// Content length

	// Verify message length definitions
	std::map<std::string, std::vector<std::string> >::const_iterator contentLengthIt = _headers.find("content-length");
	std::map<std::string, std::vector<std::string> >::const_iterator transferEncodingIt = _headers.find("transfer-encoding");

	if (contentLengthIt != _headers.end() && transferEncodingIt != _headers.end())
	{
		Logger::log(Logger::ERROR, "Malformed request: content-length and transfer-encoding headers cannot be present at the same time");
		return PARSE_ERROR_MALFORMED_REQUEST;
	}
	else if (contentLengthIt != _headers.end()) // content-length
	{
		if (contentLengthIt->second.empty())
		{
			Logger::log(Logger::ERROR, "Content length is empty");
			return PARSE_ERROR_MALFORMED_REQUEST;
		}
		else if (std::adjacent_find(contentLengthIt->second.begin(), contentLengthIt->second.end(), std::not_equal_to<char>()) != contentLengthIt->second.end())
		{
			Logger::log(Logger::ERROR, "Multiple differing definitions of content length found");
			return PARSE_ERROR_MALFORMED_REQUEST;
		}
		char *endPtr;
		_contentLength = std::strtod(contentLengthIt->second[0].c_str(), &endPtr);
		if (*endPtr != '\0')
		{
			Logger::log(Logger::ERROR, "Content length is not a valid number: " + contentLengthIt->second[0]);
			return PARSE_ERROR_MALFORMED_REQUEST;
		}
		if ((_contentLength + _bytesReceived) > _maxContentLength)
		{
			Logger::log(Logger::ERROR, "Indicated content length too long: " + contentLengthIt->second[0]);
			return PARSE_ERROR_CONTENT_LENGTH_TOO_LONG;
		}
		if (_contentLength < 0)
		{
			Logger::log(Logger::ERROR, "Indicated content length is negative: " + contentLengthIt->second[0]);
			return PARSE_ERROR_MALFORMED_REQUEST;
		}
	}
	else if (transferEncodingIt != _headers.end()) // chunked transfer-encoding
	{
		if (transferEncodingIt->second.empty())
		{
			Logger::log(Logger::ERROR, "Transfer encoding is empty");
			return PARSE_ERROR_MALFORMED_REQUEST;
		}
		else if (std::adjacent_find(transferEncodingIt->second.begin(), transferEncodingIt->second.end(), std::not_equal_to<char>()) != transferEncodingIt->second.end())
		{
			Logger::log(Logger::ERROR, "Multiple differing definitions of transfer encoding found");
			return PARSE_ERROR_MALFORMED_REQUEST;
		}
		else if (transferEncodingIt->second[0] == "chunked")
		{
			_isChunked = true;
		}
		else
		{
			Logger::log(Logger::ERROR, "Invalid transfer encoding type: " + transferEncodingIt->second[0]);
			return PARSE_ERROR_MALFORMED_REQUEST;
		}
	}

	return PARSE_BODY;
}

HttpRequest::ParseState HttpRequest::_parseBody()
{
	if (_isChunked)
	{
		_parseState = _parseChunkedBody();
		return _parseState;
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

HttpRequest::ParseState HttpRequest::_parseChunkedBody()
{
	ChunkedParser::ChunkState chunkState = _chunkParser.processBuffer(_rawBuffer);
	if (chunkState == ChunkedParser::CHUNK_COMPLETE)
	{
		_body += _chunkParser.getDecodedData();
		_rawBuffer.clear();
		return PARSE_COMPLETE;
	}
	return _parseState;
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
