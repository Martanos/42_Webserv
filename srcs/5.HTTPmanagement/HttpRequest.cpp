#include "HttpRequest.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpRequest::HttpRequest()
{
	_uri = HttpURI();
	_headers = HttpHeaders();
	_body = HttpBody();
	_currentChunkSize = 0;
	_currentChunkBytesRead = 0;
	_totalBodySize = 0;
	_usingTempFile = false;
	_tempFilePath = "";
	_tempFd = FileDescriptor(-1);
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
		_uri = rhs._uri;
		_headers = rhs._headers;
		_body = rhs._body;
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

HttpRequest::ParseState HttpRequest::parseBuffer(const std::string &buffer, HttpResponse &response)
{
	_rawBuffer += buffer;
	_bytesReceived += buffer.size();

	// TODO: Implement server identification then self check against max content length
	// if (_parseState == PARSING_BODY)
	// {
	// 	_maxContentLength = bodyBufferSize;
	// }
	// if (_bytesReceived > _maxContentLength)
	// {
	// 	return PARSE_ERROR_CONTENT_LENGTH_TOO_LONG;
	// }
	while (_parseState != PARSING_COMPLETE && _parseState != PARSING_ERROR)
	{
		switch (_parseState)
		{
		case PARSING_REQUEST_LINE:
		{
			int result = _uri.parseBuffer(_rawBuffer, response);
			if (result == HttpURI::URI_PARSING_COMPLETE)
			{
				_parseState = PARSING_HEADERS;
			}
			else if (result == HttpURI::URI_PARSING_ERROR)
			{
				_parseState = PARSING_ERROR;
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
			if (_headers.getBodyType() == HttpHeaders::BODY_TYPE_NO_BODY)
			{
				_parseState = PARSING_COMPLETE;
			}
			break;
		}
		case PARSING_BODY:
		{
			int result = _body.parseBuffer(_rawBuffer, response);
			if (result == PARSING_COMPLETE)
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
	}
	return _parseState;
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
