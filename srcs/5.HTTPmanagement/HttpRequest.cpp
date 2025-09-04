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
	else if (version != HTTP::HTTP_VERSION)
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

	// line cannot contain whitespace before the colon
	if (line.find(' ') != std::string::npos)
	{
		Logger::log(Logger::ERROR, "Invalid header line: " + line);
		return PARSE_ERROR_MALFORMED_REQUEST;
	}
	// Extract the header name
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
	name = line.substr(0, colonPos);

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
	// RFC 9110 Section 5.4: Server MUST NOT apply request until entire header section received
	// This validation happens after all headers are parsed

	// === REQUIRED HEADERS VALIDATION ===

	// RFC 9112: Host header is REQUIRED for HTTP/1.1
	std::map<std::string, std::vector<std::string> >::const_iterator hostIt = _headers.find("host");
	if (hostIt == _headers.end())
	{
		Logger::log(Logger::ERROR, "Host header is missing (required for HTTP/1.1)");
		return PARSE_ERROR_MALFORMED_REQUEST;
	}
	if (hostIt->second.empty())
	{
		Logger::log(Logger::ERROR, "Host header is empty");
		return PARSE_ERROR_MALFORMED_REQUEST;
	}
	// RFC: Host is singleton header - multiple different values forbidden
	if (hostIt->second.size() > 1)
	{
		// Check if all values are identical (allowed as per RFC 9110 Section 5.3)
		std::string firstHost = hostIt->second[0];
		for (size_t i = 1; i < hostIt->second.size(); ++i)
		{
			if (hostIt->second[i] != firstHost)
			{
				Logger::log(Logger::ERROR, "Multiple different Host header values found");
				return PARSE_ERROR_MALFORMED_REQUEST;
			}
		}
	}

	// === MESSAGE BODY LENGTH VALIDATION ===

	std::map<std::string, std::vector<std::string> >::const_iterator contentLengthIt = _headers.find("content-length");
	std::map<std::string, std::vector<std::string> >::const_iterator transferEncodingIt = _headers.find("transfer-encoding");

	// RFC 9110 Section 8.6: Content-Length and Transfer-Encoding MUST NOT coexist
	if (contentLengthIt != _headers.end() && transferEncodingIt != _headers.end())
	{
		Logger::log(Logger::ERROR, "Content-Length and Transfer-Encoding cannot both be present");
		return PARSE_ERROR_MALFORMED_REQUEST;
	}

	// === CONTENT-LENGTH VALIDATION ===
	if (contentLengthIt != _headers.end())
	{
		if (contentLengthIt->second.empty())
		{
			Logger::log(Logger::ERROR, "Content-Length header is empty");
			return PARSE_ERROR_MALFORMED_REQUEST;
		}

		// RFC 9110 Section 8.6: Multiple identical values allowed, different values forbidden
		std::string firstValue = contentLengthIt->second[0];
		for (size_t i = 1; i < contentLengthIt->second.size(); ++i)
		{
			if (contentLengthIt->second[i] != firstValue)
			{
				Logger::log(Logger::ERROR, "Multiple different Content-Length values: " +
											   firstValue + " vs " + contentLengthIt->second[i]);
				return PARSE_ERROR_MALFORMED_REQUEST;
			}
		}

		// Validate the numeric value
		char *endPtr;
		_contentLength = std::strtod(firstValue.c_str(), &endPtr);
		if (*endPtr != '\0')
		{
			Logger::log(Logger::ERROR, "Content-Length is not a valid number: " + firstValue);
			return PARSE_ERROR_MALFORMED_REQUEST;
		}
		if (_contentLength < 0)
		{
			Logger::log(Logger::ERROR, "Content-Length cannot be negative: " + firstValue);
			return PARSE_ERROR_MALFORMED_REQUEST;
		}

		// Check against maximum allowed content length
		if ((_contentLength + _bytesReceived) > _maxContentLength)
		{
			Logger::log(Logger::ERROR, "Content-Length exceeds maximum allowed: " + firstValue);
			return PARSE_ERROR_CONTENT_LENGTH_TOO_LONG;
		}
	}

	// === TRANSFER-ENCODING VALIDATION ===
	else if (transferEncodingIt != _headers.end())
	{
		if (transferEncodingIt->second.empty())
		{
			Logger::log(Logger::ERROR, "Transfer-Encoding header is empty");
			return PARSE_ERROR_MALFORMED_REQUEST;
		}

		// RFC: Transfer-Encoding can have multiple encodings, but for WebServ we only support chunked
		// The last encoding MUST be "chunked" if present
		bool hasChunked = false;
		for (size_t i = 0; i < transferEncodingIt->second.size(); ++i)
		{
			std::string encoding = _toLowerCase(transferEncodingIt->second[i]);
			if (encoding == "chunked")
			{
				hasChunked = true;
				// RFC: chunked MUST be the final encoding
				if (i != transferEncodingIt->second.size() - 1)
				{
					Logger::log(Logger::ERROR, "chunked encoding must be the final encoding");
					return PARSE_ERROR_MALFORMED_REQUEST;
				}
			}
			else
			{
				// WebServ only supports chunked encoding
				Logger::log(Logger::ERROR, "Unsupported transfer encoding: " + encoding);
				return PARSE_ERROR_MALFORMED_REQUEST;
			}
		}

		if (hasChunked)
		{
			_isChunked = true;
		}
		else
		{
			Logger::log(Logger::ERROR, "Transfer-Encoding present but no chunked encoding found");
			return PARSE_ERROR_MALFORMED_REQUEST;
		}
	}

	// === VALIDATE SINGLETON HEADERS ===
	// These headers should only have one semantic value
	const size_t singletonCount = sizeof(HTTP::SINGLETON_HEADERS) / sizeof(HTTP::SINGLETON_HEADERS[0]);
	for (size_t h = 0; h < singletonCount; ++h)
	{
		std::map<std::string, std::vector<std::string> >::const_iterator it =
			_headers.find(HTTP::SINGLETON_HEADERS[h]);
		if (it != _headers.end() && it->second.size() > 1)
		{
			// Check if values are identical (some allow duplicates if identical)
			std::string firstValue = it->second[0];
			bool allIdentical = true;
			for (size_t i = 1; i < it->second.size(); ++i)
			{
				if (it->second[i] != firstValue)
				{
					allIdentical = false;
					break;
				}
			}
			if (!allIdentical)
			{
				Logger::log(Logger::WARNING, "Multiple different values for singleton header: " +
												 std::string(HTTP::SINGLETON_HEADERS[h]));
				// RFC suggests using last valid value for Content-Type, but we'll warn and continue
			}
		}
	}

	// === VALIDATE DANGEROUS CHARACTERS ===
	// RFC 9110 Section 5.5: Field values containing CR, LF, or NUL are invalid and dangerous
	for (std::map<std::string, std::vector<std::string> >::const_iterator headerIt = _headers.begin();
		 headerIt != _headers.end(); ++headerIt)
	{
		for (size_t i = 0; i < headerIt->second.size(); ++i)
		{
			const std::string &value = headerIt->second[i];
			for (size_t j = 0; j < value.size(); ++j)
			{
				char c = value[j];
				if (c == '\r' || c == '\n' || c == '\0')
				{
					Logger::log(Logger::ERROR, "Dangerous character in header " +
												   headerIt->first + ": CR/LF/NUL detected");
					return PARSE_ERROR_MALFORMED_REQUEST;
				}
				// RFC: Other control characters should be treated carefully
				if (c > 0 && c < 32 && c != '\t')
				{
					Logger::log(Logger::WARNING, "Control character in header " +
													 headerIt->first + " (char code: " + StringUtils::toString((int)c) + ")");
				}
			}
		}
	}

	// === CHECK FIELD SIZE LIMITS ===
	// RFC 9110 Section 5.4: Server that receives oversized fields MUST respond with 4xx
	size_t totalHeaderSize = 0;
	for (std::map<std::string, std::vector<std::string> >::const_iterator headerIt = _headers.begin();
		 headerIt != _headers.end(); ++headerIt)
	{
		totalHeaderSize += headerIt->first.size() + 2; // +2 for ": "
		for (size_t i = 0; i < headerIt->second.size(); ++i)
		{
			totalHeaderSize += headerIt->second[i].size();
			if (i > 0)
				totalHeaderSize += 2; // +2 for ", "
		}
		totalHeaderSize += 2; // +2 for CRLF
	}

	if (totalHeaderSize > _maxHeaderSize)
	{
		Logger::log(Logger::ERROR, "Total header size exceeds limit: " + StringUtils::toString(totalHeaderSize) + " > " + StringUtils::toString(_maxHeaderSize));
		return PARSE_ERROR_HEADER_TOO_LONG;
	}

	Logger::log(Logger::INFO, "Headers parsed successfully. Total headers: " +
								  StringUtils::toString(_headers.size()) + ", Total size: " + StringUtils::toString(totalHeaderSize));

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
		size_t needed = _contentLength - _bytesReceived;
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
	return std::find(HTTP::SUPPORTED_METHODS, HTTP::SUPPORTED_METHODS + 4, method) != HTTP::SUPPORTED_METHODS + 4;
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
