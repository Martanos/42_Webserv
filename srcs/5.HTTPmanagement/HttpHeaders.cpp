#include "HttpHeaders.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpHeaders::HttpHeaders()
{
	_headersState = HEADERS_PARSING;
	_rawHeaders = RingBuffer(sysconf(_SC_PAGE_SIZE) * 2); // Should equal 32KB/64KB depending on the system
	_headers.clear();
}

HttpHeaders::HttpHeaders(const HttpHeaders &src)
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
		this->_headersState = rhs._headersState;
		this->_rawHeaders = rhs._rawHeaders;
		this->_headers = rhs._headers;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

int HttpHeaders::parseBuffer(RingBuffer &buffer, HttpResponse &response, HttpBody &body)
{
	size_t newlinePos = _rawHeaders.contains("\r\n", 2);
	if (newlinePos == _rawHeaders.capacity())
	{
		if (buffer.readable() > sysconf(_SC_PAGE_SIZE))
		{
			Logger::log(Logger::ERROR, "Headers too long");
			response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
			response.setStatusMessage("Bad Request");
			response.setBody("Bad Request");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
			response.setHeader("Connection", "close");
			return _headersState;
		}
		return _headersState;
	}
	std::string headerLine;
	_rawHeaders.transferFrom(buffer, newlinePos);
	_rawHeaders.peekBuffer(headerLine, _rawHeaders.readable());
	parseHeaderLine(headerLine, response, body);
	return _headersState;
}

void HttpHeaders::parseHeaderLine(const std::string &line, HttpResponse &response, HttpBody &body)
{
	// Extract the header and attempt to insert into headers map
	std::stringstream ss(line);
	std::string name, token;

	// line cannot contain whitespace before the colon
	if (line.find(' ') != std::string::npos)
	{
		Logger::log(Logger::ERROR, "Invalid header line: " + line);
		response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
		response.setStatusMessage("Bad Request");
		response.setBody("Bad Request");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		_headersState = HEADERS_PARSING_ERROR;
		return;
	}
	// Check if this is the CLRF termination line
	if (line == "\r\n")
		return parseHeaders(response, body);
	// Extract the header name
	if (!(ss >> name))
	{
		Logger::log(Logger::ERROR, "Internal server error: " + line);
		response.setStatusCode(HTTP::STATUS_INTERNAL_SERVER_ERROR);
		response.setStatusMessage("Internal Server Error");
		response.setBody("Internal Server Error");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		_headersState = HEADERS_PARSING_ERROR;
		return;
	}
	ss >> name;
	size_t colonPos = name.find(':');
	if (colonPos == std::string::npos)
	{
		Logger::log(Logger::ERROR, "Invalid header line: " + line);
		response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
		response.setStatusMessage("Bad Request");
		response.setBody("Bad Request");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		_headersState = HEADERS_PARSING_ERROR;
		return;
	}
	name = line.substr(0, colonPos);

	// Find if header already exists, if it doesnt create new key with empty vector
	std::map<std::string, std::vector<std::string> >::iterator it = _headers.find(StringUtils::toLowerCase(name));
	if (it == _headers.end())
	{
		it = _headers.insert(std::make_pair(StringUtils::toLowerCase(name), std::vector<std::string>())).first;
		if (it == _headers.end())
		{
			Logger::log(Logger::ERROR, "Internal server error: " + line);
			response.setStatusCode(HTTP::STATUS_INTERNAL_SERVER_ERROR);
			response.setStatusMessage("Internal Server Error");
			response.setBody("Internal Server Error");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
			response.setHeader("Connection", "close");
			_headersState = HEADERS_PARSING_ERROR;
			return;
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
	return;
}

void HttpHeaders::parseHeaders(HttpResponse &response, HttpBody &body)
{
	// RFC 9110 Section 5.4: Server MUST NOT apply request until entire header section received
	// This validation happens after all headers are parsed

	// === REQUIRED HEADERS VALIDATION ===

	// RFC 9112: Host header is REQUIRED for HTTP/1.1
	std::map<std::string, std::vector<std::string> >::const_iterator hostIt = _headers.find("host");
	if (hostIt == _headers.end())
	{
		Logger::log(Logger::ERROR, "Host header is missing (required for HTTP/1.1)");
		response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
		response.setStatusMessage("Bad Request");
		response.setBody("Bad Request");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		_headersState = HEADERS_PARSING_ERROR;
		return;
	}
	if (hostIt->second.empty())
	{
		Logger::log(Logger::ERROR, "Host header is empty");
		response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
		response.setStatusMessage("Bad Request");
		response.setBody("Bad Request");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		_headersState = HEADERS_PARSING_ERROR;
		return;
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
				response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
				response.setStatusMessage("Bad Request");
				response.setBody("Bad Request");
				response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
				response.setHeader("Connection", "close");
				_headersState = HEADERS_PARSING_ERROR;
				return;
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
		response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
		response.setStatusMessage("Bad Request");
		response.setBody("Bad Request");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		_headersState = HEADERS_PARSING_ERROR;
		return;
	}

	// === CONTENT-LENGTH VALIDATION ===
	if (contentLengthIt != _headers.end())
	{
		if (contentLengthIt->second.empty())
		{
			Logger::log(Logger::ERROR, "Content-Length header is empty");
			response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
			response.setStatusMessage("Bad Request");
			response.setBody("Bad Request");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
			response.setHeader("Connection", "close");
			_headersState = HEADERS_PARSING_ERROR;
			return;
		}

		// RFC 9110 Section 8.6: Multiple identical values allowed, different values forbidden
		std::string firstValue = contentLengthIt->second[0];
		for (size_t i = 1; i < contentLengthIt->second.size(); ++i)
		{
			if (contentLengthIt->second[i] != firstValue)
			{
				Logger::log(Logger::ERROR, "Multiple different Content-Length values: " +
											   firstValue + " vs " + contentLengthIt->second[i]);
				response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
				response.setStatusMessage("Bad Request");
				response.setBody("Bad Request");
				response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
				response.setHeader("Connection", "close");
				_headersState = HEADERS_PARSING_ERROR;
				return;
			}
		}

		// Validate the numeric value
		char *endPtr;
		double contentLength = std::strtod(firstValue.c_str(), &endPtr);
		if (*endPtr != '\0')
		{
			Logger::log(Logger::ERROR, "Content-Length is not a valid number: " + firstValue);
			response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
			response.setStatusMessage("Bad Request");
			response.setBody("Bad Request");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
			response.setHeader("Connection", "close");
			_headersState = HEADERS_PARSING_ERROR;
			return;
		}
		else if (contentLength < 0)
		{
			Logger::log(Logger::ERROR, "Content-Length cannot be negative: " + firstValue);
			response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
			response.setStatusMessage("Bad Request");
			response.setBody("Bad Request");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
			response.setHeader("Connection", "close");
			_headersState = HEADERS_PARSING_ERROR;
			return;
		}
		else if (contentLength > std::numeric_limits<int64_t>::max())
		{
			Logger::log(Logger::ERROR, "Content-Length is too large: " + firstValue);
			response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
			response.setStatusMessage("Bad Request");
			response.setBody("Bad Request");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
			response.setHeader("Connection", "close");
			_headersState = HEADERS_PARSING_ERROR;
			return;
		}
		else
		{
			if (contentLength == 0)
			{
				body.setBodyType(HttpBody::BODY_TYPE_NO_BODY);
				return;
			}
			else
			{
				body.setExpectedBodySize(contentLength);
				body.setBodyType(HttpBody::BODY_TYPE_CONTENT_LENGTH);
				return;
			}
		}
	}

	// === TRANSFER-ENCODING VALIDATION ===
	else if (transferEncodingIt != _headers.end())
	{
		if (transferEncodingIt->second.empty())
		{
			Logger::log(Logger::ERROR, "Transfer-Encoding header is empty");
			response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
			response.setStatusMessage("Bad Request");
			response.setBody("Bad Request");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
			response.setHeader("Connection", "close");
			_headersState = HEADERS_PARSING_ERROR;
			return;
		}

		// RFC: Transfer-Encoding can have multiple encodings, but for WebServ we only support chunked
		// The last encoding MUST be "chunked" if present
		bool hasChunked = false;
		for (size_t i = 0; i < transferEncodingIt->second.size(); ++i)
		{
			std::string encoding = StringUtils::toLowerCase(transferEncodingIt->second[i]);
			if (encoding == "chunked")
			{
				hasChunked = true;
				// RFC: chunked MUST be the final encoding
				if (i != transferEncodingIt->second.size() - 1)
				{
					Logger::log(Logger::ERROR, "chunked encoding must be the final encoding");
					response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
					response.setStatusMessage("Bad Request");
					response.setBody("Bad Request");
					response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
					response.setHeader("Connection", "close");
					_headersState = HEADERS_PARSING_ERROR;
					return;
				}
			}
			else
			{
				// WebServ only supports chunked encoding
				Logger::log(Logger::ERROR, "Unsupported transfer encoding: " + encoding);
				response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
				response.setStatusMessage("Bad Request");
				response.setBody("Bad Request");
				response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
				response.setHeader("Connection", "close");
				_headersState = HEADERS_PARSING_ERROR;
				return;
			}
		}
		if (!hasChunked)
		{
			Logger::log(Logger::ERROR, "Transfer-Encoding present but no chunked encoding found");
			response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
			response.setStatusMessage("Bad Request");
			response.setBody("Bad Request");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
			response.setHeader("Connection", "close");
			_headersState = HEADERS_PARSING_ERROR;
			return;
		}
		else
		{
			body.setExpectedBodySize(HTTP::MAX_BODY_SIZE);
			body.setBodyType(HttpBody::BODY_TYPE_CHUNKED);
			return;
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
					response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
					response.setStatusMessage("Bad Request");
					response.setBody("Bad Request");
					response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
					response.setHeader("Connection", "close");
					_headersState = HEADERS_PARSING_ERROR;
					return;
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

	if (totalHeaderSize > (sysconf(_SC_PAGE_SIZE) * 2))
	{
		Logger::log(Logger::ERROR, "Total header size exceeds limit: " + StringUtils::toString(totalHeaderSize) + " > " + StringUtils::toString(sysconf(_SC_PAGE_SIZE) * 2));
		response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
		response.setStatusMessage("Bad Request");
		response.setBody("Bad Request");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		_headersState = HEADERS_PARSING_ERROR;
		return;
	}

	Logger::log(Logger::INFO, "Headers parsed successfully. Total headers: " +
								  StringUtils::toString(_headers.size()) + ", Total size: " + StringUtils::toString(totalHeaderSize));
	_headersState = HEADERS_PARSING_COMPLETE;
	return;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

int HttpHeaders::getHeadersState() const
{
	return _headersState;
}

RingBuffer HttpHeaders::getRawHeaders() const
{
	return _rawHeaders;
}

std::map<std::string, std::vector<std::string> > HttpHeaders::getHeaders() const
{
	return _headers;
}

size_t HttpHeaders::getHeadersSize() const
{
	return _headers.size();
}

size_t HttpHeaders::getExpectedBodySize() const
{
	return _expectedBodySize;
}

/*
** --------------------------------- MUTATOR ---------------------------------
*/

void HttpHeaders::setHeadersState(HeadersState headersState)
{
	_headersState = headersState;
}

void HttpHeaders::setRawHeaders(const RingBuffer &rawHeaders)
{
	_rawHeaders.writeBuffer(rawHeaders, rawHeaders.readable());
}

void HttpHeaders::setHeaders(const std::map<std::string, std::vector<std::string> > &headers)
{
	_headers = headers;
}

void HttpHeaders::setExpectedBodySize(size_t expectedBodySize)
{
	_expectedBodySize = expectedBodySize;
}

void HttpHeaders::reset()
{
	_expectedBodySize = 0;
	_headersState = HEADERS_PARSING;
	_rawHeaders.reset();
	_headers.clear();
}

/* ************************************************************************** */
