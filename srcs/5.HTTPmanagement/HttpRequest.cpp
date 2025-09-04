#include "HttpRequest.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpRequest::HttpRequest()
{
	_method = "";
	_uri = "";
	_version = "";
	_headers = std::map<std::string, std::vector<std::string> >();
	_body = "";
	_contentLength = 0;
	_isChunked = false;
	_chunkState = CHUNK_SIZE;
	_currentChunkSize = 0;
	_currentChunkBytesRead = 0;
	_totalBodySize = 0;
	_usingTempFile = false;
	_tempFilePath = "";
	_tempFd = FileDescriptor(-1);
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
	_cleanupTempFile();
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

HttpRequest &HttpRequest::operator=(const HttpRequest &rhs)
{
	if (this != &rhs)
	{
		// Clean up existing temp file if any
		_cleanupTempFile();

		// Copy basic members
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

		// Copy chunked parsing state
		_chunkState = rhs._chunkState;
		_currentChunkSize = rhs._currentChunkSize;
		_currentChunkBytesRead = rhs._currentChunkBytesRead;
		_totalBodySize = rhs._totalBodySize;
		_usingTempFile = rhs._usingTempFile;
		_tempFilePath = rhs._tempFilePath;

		// Note: File descriptor is not copied - temp files are per-request
		_tempFd = FileDescriptor(-1);
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

	if (_parseState == PARSE_BODY)
	{
		_maxContentLength = bodyBufferSize;
	}
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
			size_t newlinePos = _rawBuffer.find("\r\n");
			if (newlinePos == std::string::npos)
			{
				return _parseState;
			}
			std::string requestLine = _rawBuffer.substr(0, newlinePos);
			if (requestLine.size() > _maxRequestLineSize)
			{
				return PARSE_ERROR_REQUEST_LINE_TOO_LONG;
			}
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
			size_t newlinePos = _rawBuffer.find("\r\n");
			if (newlinePos == std::string::npos)
			{
				return _parseState;
			}
			std::string headerLine = _rawBuffer.substr(0, newlinePos);
			if (headerLine.size() > _maxHeaderSize)
			{
				return PARSE_ERROR_HEADER_TOO_LONG;
			}
			_rawBuffer.erase(0, newlinePos + 2);
			if (headerLine.empty())
			{
				// Empty line indicates end of headers
				_parseState = _parseHeaders();
				if (_parseState != PARSE_BODY)
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
			_parseState = _parseBody();
			break;
		}
		default:
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
		return _parseChunkedBody();
	}
	else if (_contentLength > 0)
	{
		size_t needed = _contentLength - _totalBodySize;
		size_t available = _rawBuffer.length();
		size_t toRead = (needed < available) ? needed : available;

		if (toRead > 0)
		{
			std::string data = _rawBuffer.substr(0, toRead);
			_rawBuffer.erase(0, toRead);

			ParseState result = _handleBodyData(data);
			if (result != PARSE_BODY)
			{
				return result;
			}
		}

		if (_totalBodySize >= _contentLength)
		{
			Logger::log(Logger::INFO, "Body parsing complete. Total size: " +
										  StringUtils::toString(_totalBodySize));
			return PARSE_COMPLETE;
		}
	}
	else if (_contentLength == 0 && !_isChunked)
	{
		// No body expected
		return PARSE_COMPLETE;
	}

	return PARSE_BODY;
}

HttpRequest::ParseState HttpRequest::_parseChunkedBody()
{
	while (!_rawBuffer.empty())
	{
		switch (_chunkState)
		{
		case CHUNK_SIZE:
		{
			ParseState result = _parseChunkSize();
			if (result != PARSE_BODY)
			{
				return result;
			}
			break;
		}
		case CHUNK_DATA:
		{
			ParseState result = _parseChunkData();
			if (result != PARSE_BODY)
			{
				return result;
			}
			break;
		}
		case CHUNK_TRAILER:
		{
			ParseState result = _parseChunkTrailers();
			if (result != PARSE_BODY)
			{
				return result;
			}
			break;
		}
		case CHUNK_COMPLETE:
		{
			Logger::log(Logger::INFO, "Chunked transfer complete. Total size: " +
										  StringUtils::toString(_totalBodySize));
			return PARSE_COMPLETE;
		}
		}
	}

	return PARSE_BODY;
}

HttpRequest::ParseState HttpRequest::_parseChunkSize()
{
	size_t crlfPos = _rawBuffer.find("\r\n");
	if (crlfPos == std::string::npos)
	{
		return PARSE_BODY; // Need more data
	}

	std::string sizeLine = _rawBuffer.substr(0, crlfPos);
	_rawBuffer.erase(0, crlfPos + 2);

	// Handle chunk extensions - ignore them per RFC
	size_t semicolonPos = sizeLine.find(';');
	if (semicolonPos != std::string::npos)
	{
		sizeLine = sizeLine.substr(0, semicolonPos);
	}

	// Parse hexadecimal chunk size
	_currentChunkSize = _parseHexSize(sizeLine);
	if (_currentChunkSize == static_cast<size_t>(-1))
	{
		Logger::log(Logger::ERROR, "Invalid chunk size: " + sizeLine);
		return PARSE_ERROR_MALFORMED_REQUEST;
	}

	_currentChunkBytesRead = 0;

	if (_currentChunkSize == 0)
	{
		// Last chunk - move to trailer parsing
		_chunkState = CHUNK_TRAILER;
		Logger::log(Logger::DEBUG, "Last chunk received");
	}
	else
	{
		// Regular chunk - move to data parsing
		_chunkState = CHUNK_DATA;
		Logger::log(Logger::DEBUG, "Reading chunk of size: " +
									   StringUtils::toString(_currentChunkSize));
	}

	return PARSE_BODY;
}

HttpRequest::ParseState HttpRequest::_parseChunkData()
{
	size_t remaining = _currentChunkSize - _currentChunkBytesRead;
	size_t available = _rawBuffer.length();

	// We need chunk data + CRLF
	if (available < remaining + 2 && _currentChunkBytesRead + available < _currentChunkSize)
	{
		// Not enough data yet, consume what we have
		if (available > 0)
		{
			ParseState result = _handleBodyData(_rawBuffer);
			if (result != PARSE_BODY)
			{
				return result;
			}

			_currentChunkBytesRead += available;
			_rawBuffer.clear();
		}
		return PARSE_BODY;
	}

	// Extract chunk data
	size_t toRead = remaining;
	if (toRead > available)
	{
		toRead = available;
	}

	std::string chunkData = _rawBuffer.substr(0, toRead);
	_rawBuffer.erase(0, toRead);
	_currentChunkBytesRead += toRead;

	// Store chunk data
	ParseState result = _handleBodyData(chunkData);
	if (result != PARSE_BODY)
	{
		return result;
	}

	// Check if we've read the complete chunk
	if (_currentChunkBytesRead >= _currentChunkSize)
	{
		// Consume trailing CRLF
		if (_rawBuffer.length() >= 2)
		{
			if (_rawBuffer.substr(0, 2) != "\r\n")
			{
				Logger::log(Logger::ERROR, "Missing CRLF after chunk data");
				return PARSE_ERROR_MALFORMED_REQUEST;
			}
			_rawBuffer.erase(0, 2);
			_chunkState = CHUNK_SIZE; // Ready for next chunk
		}
		else
		{
			// Need more data for CRLF
			return PARSE_BODY;
		}
	}

	return PARSE_BODY;
}

// Parse chunk trailers (after last chunk)
HttpRequest::ParseState HttpRequest::_parseChunkTrailers()
{
	// Look for final CRLF indicating end of trailers
	size_t crlfPos = _rawBuffer.find("\r\n");
	if (crlfPos == std::string::npos)
	{
		return PARSE_BODY; // Need more data
	}

	std::string trailerLine = _rawBuffer.substr(0, crlfPos);
	_rawBuffer.erase(0, crlfPos + 2);

	if (trailerLine.empty())
	{
		// Empty line indicates end of trailers
		_chunkState = CHUNK_COMPLETE;
		Logger::log(Logger::INFO, "Chunked encoding complete");
	}
	else
	{
		// Process trailer header (could be important headers like Content-MD5)
		// For WebServ, we'll log and ignore per RFC guidelines
		Logger::log(Logger::DEBUG, "Ignoring trailer header: " + trailerLine);
	}

	return PARSE_BODY;
}

// Switch from memory to temp file storage
HttpRequest::ParseState HttpRequest::_switchToTempFile()
{
	if (_usingTempFile)
	{
		return PARSE_BODY; // Already using temp file
	}

	Logger::log(Logger::INFO, "Switching to temp file storage. Current size: " +
								  StringUtils::toString(_totalBodySize));

	// Create temp file
	_tempFilePath = _createTempFilePath();
	char tempPath[256];
	std::strcpy(tempPath, _tempFilePath.c_str());

	int fd = mkstemp(tempPath);
	if (fd == -1)
	{
		Logger::log(Logger::ERROR, "Failed to create temp file: " + std::string(strerror(errno)));
		return PARSE_ERROR_TEMP_FILE_ERROR;
	}

	_tempFd = FileDescriptor(fd);
	_tempFilePath = tempPath;
	_usingTempFile = true;

	// Write existing body data to temp file
	if (!_body.empty())
	{
		ssize_t written = _tempFd.writeFile(_body);
		if (written < 0 || static_cast<size_t>(written) != _body.size())
		{
			Logger::log(Logger::ERROR, "Failed to write existing body to temp file");
			_cleanupTempFile();
			return PARSE_ERROR_TEMP_FILE_ERROR;
		}
		_body.clear(); // Clear memory copy
	}

	Logger::log(Logger::INFO, "Successfully switched to temp file: " + _tempFilePath);
	return PARSE_BODY;
}

// Append data to temp file
HttpRequest::ParseState HttpRequest::_appendToTempFile(const std::string &data)
{
	if (!_usingTempFile || _tempFd.getFd() == -1)
	{
		Logger::log(Logger::ERROR, "Temp file not properly initialized");
		return PARSE_ERROR_TEMP_FILE_ERROR;
	}

	ssize_t written = _tempFd.writeFile(data);
	if (written < 0 || static_cast<size_t>(written) != data.size())
	{
		Logger::log(Logger::ERROR, "Failed to write to temp file: " + std::string(strerror(errno)));
		return PARSE_ERROR_TEMP_FILE_ERROR;
	}

	_totalBodySize += data.length();

	Logger::log(Logger::DEBUG, "Wrote " + StringUtils::toString(data.length()) +
								   " bytes to temp file. Total: " + StringUtils::toString(_totalBodySize));

	return PARSE_BODY;
}

// Parse hexadecimal chunk size
size_t HttpRequest::_parseHexSize(const std::string &hexStr) const
{
	if (hexStr.empty())
	{
		return static_cast<size_t>(-1);
	}

	size_t result = 0;
	for (size_t i = 0; i < hexStr.length(); ++i)
	{
		char c = hexStr[i];
		int digit = -1;

		if (c >= '0' && c <= '9')
		{
			digit = c - '0';
		}
		else if (c >= 'A' && c <= 'F')
		{
			digit = c - 'A' + 10;
		}
		else if (c >= 'a' && c <= 'f')
		{
			digit = c - 'a' + 10;
		}
		else
		{
			// Invalid hex character
			return static_cast<size_t>(-1);
		}

		// Check for overflow
		if (result > (SIZE_MAX - digit) / 16)
		{
			Logger::log(Logger::ERROR, "Chunk size too large: " + hexStr);
			return static_cast<size_t>(-1);
		}

		result = result * 16 + digit;
	}

	return result;
}

// Create temp file path
std::string HttpRequest::_createTempFilePath() const
{
	std::time_t now = std::time(0);
	char buf[80];
	std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", std::localtime(&now));
	return std::string(HTTP::TEMP_FILE_TEMPLATE) + std::string(buf);
}

// Clean up temp file
void HttpRequest::_cleanupTempFile()
{
	if (_usingTempFile && !_tempFilePath.empty())
	{
		if (_tempFd.getFd() != -1)
		{
			// File descriptor will be closed by destructor
		}

		// Remove temp file
		if (unlink(_tempFilePath.c_str()) != 0)
		{
			Logger::log(Logger::WARNING, "Failed to remove temp file: " + _tempFilePath);
		}
		else
		{
			Logger::log(Logger::DEBUG, "Removed temp file: " + _tempFilePath);
		}

		_tempFilePath.clear();
		_usingTempFile = false;
	}
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

const std::vector<std::string> &HttpRequest::getHeader(const std::string &name) const
{
	static const std::vector<std::string> empty = std::vector<std::string>();
	std::map<std::string, std::vector<std::string> >::const_iterator it = _headers.find(_toLowerCase(name));
	return it != _headers.end() ? it->second : empty;
}

// Get body from temp file
std::string HttpRequest::_getBodyFromFile() const
{
	if (!_usingTempFile || _tempFilePath.empty())
	{
		return _body; // Return in-memory body
	}

	// Read from temp file
	FileDescriptor readFd(open(_tempFilePath.c_str(), O_RDONLY));
	if (readFd.getFd() == -1)
	{
		Logger::log(Logger::ERROR, "Failed to open temp file for reading: " + _tempFilePath);
		return "";
	}

	std::string content;
	content.resize(_totalBodySize);

	ssize_t bytesRead = read(readFd.getFd(), &content[0], _totalBodySize);
	if (bytesRead < 0 || static_cast<size_t>(bytesRead) != _totalBodySize)
	{
		Logger::log(Logger::ERROR, "Failed to read complete temp file");
		return "";
	}

	return content;
}

// Enhanced reset method
void HttpRequest::reset()
{
	_headers.clear();
	_body.clear();
	_contentLength = 0;
	_isChunked = false;
	_parseState = PARSE_REQUEST_LINE;
	_rawBuffer.clear();
	_bytesReceived = 0;

	// Reset chunked parsing state
	_chunkState = CHUNK_SIZE;
	_currentChunkSize = 0;
	_currentChunkBytesRead = 0;
	_totalBodySize = 0;
	_usingTempFile = false;
	_tempFilePath.clear();
	_tempFd = FileDescriptor(-1);
}

const std::string &HttpRequest::getMethod() const { return _method; };
const std::string &HttpRequest::getUri() const { return _uri; }
const std::string &HttpRequest::getVersion() const { return _version; };
const std::map<std::string, std::vector<std::string> > &HttpRequest::getHeaders() const { return _headers; };
const std::string &HttpRequest::getBody() const { return _body; };
size_t HttpRequest::getContentLength() const { return _contentLength; };
bool HttpRequest::isChunked() const { return _isChunked; };
bool HttpRequest::isUsingTempFile() const { return _usingTempFile; };
std::string HttpRequest::getTempFile() const { return _tempFilePath; };
