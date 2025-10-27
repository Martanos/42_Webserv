<<<<<<< HEAD
#include "../../includes/HttpBody.hpp"
#include "../../includes/HttpResponse.hpp"
#include "../../includes/Logger.hpp"
#include "../../includes/StringUtils.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpBody::HttpBody() : _bodyState(BODY_PARSING), _bodyType(BODY_TYPE_NO_BODY), 
	_chunkState(CHUNK_SIZE), _expectedBodySize(0), _chunkedBuffer(), _rawBody(), 
	_tempChunkedFile(), _tempFile(), _isUsingTempFile(false)
{
}

HttpBody::HttpBody(HttpBody const &src) : _bodyState(src._bodyState), 
	_bodyType(src._bodyType), _chunkState(src._chunkState), 
	_expectedBodySize(src._expectedBodySize), _chunkedBuffer(src._chunkedBuffer), 
	_rawBody(src._rawBody), _tempChunkedFile(src._tempChunkedFile), 
	_tempFile(src._tempFile), _isUsingTempFile(src._isUsingTempFile)
{
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

HttpBody::~HttpBody()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

HttpBody &HttpBody::operator=(HttpBody const &rhs)
{
	if (this != &rhs)
	{
		_bodyState = rhs._bodyState;
		_bodyType = rhs._bodyType;
		_chunkState = rhs._chunkState;
		_expectedBodySize = rhs._expectedBodySize;
		_chunkedBuffer = rhs._chunkedBuffer;
		_rawBody = rhs._rawBody;
		_tempChunkedFile = rhs._tempChunkedFile;
		_tempFile = rhs._tempFile;
		_isUsingTempFile = rhs._isUsingTempFile;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

int HttpBody::parseBuffer(RingBuffer &buffer, HttpResponse &response)
{
	if (_bodyState == BODY_PARSING_COMPLETE)
		return BODY_PARSING_COMPLETE;

	if (_bodyState == BODY_PARSING_ERROR)
		return BODY_PARSING_ERROR;

	if (_bodyType == BODY_TYPE_NO_BODY)
	{
		_bodyState = BODY_PARSING_COMPLETE;
		return BODY_PARSING_COMPLETE;
	}
	else if (_bodyType == BODY_TYPE_CHUNKED)
	{
		BodyState result = _parseChunkedBody(buffer, response);
		_bodyState = result;
		return result;
	}
	else if (_bodyType == BODY_TYPE_CONTENT_LENGTH)
	{
		BodyState result = _parseContentLengthBody(buffer, response);
		_bodyState = result;
		return result;
	}

	_bodyState = BODY_PARSING_ERROR;
	return BODY_PARSING_ERROR;
}

HttpBody::BodyState HttpBody::_parseChunkedBody(RingBuffer &buffer, HttpResponse &response)
{
	(void)response; // TODO: Use response for error handling
	while (buffer.readable() > 0)
	{
		switch (_chunkState)
		{
		case CHUNK_SIZE:
		{
			std::string line;
			buffer.peekBuffer(line, buffer.readable());
			
			size_t crlfPos = line.find("\r\n");
			if (crlfPos == std::string::npos)
			{
				crlfPos = line.find('\n');
				if (crlfPos == std::string::npos)
				{
					// Need more data
					return BODY_PARSING;
				}
				crlfPos += 1; // Include the \n
			}
			else
			{
				crlfPos += 2; // Include the \r\n
			}

			std::string sizeLine = line.substr(0, crlfPos - 2);
			if (sizeLine.empty())
			{
				Logger::log(Logger::ERROR, "Empty chunk size line");
				return BODY_PARSING_ERROR;
			}

			size_t chunkSize = _parseHexSize(sizeLine);
			if (chunkSize == 0)
			{
				// End of chunks
				_chunkState = CHUNK_COMPLETE;
				buffer.consume(crlfPos);
				continue;
			}

			_expectedBodySize = chunkSize;
			_chunkState = CHUNK_DATA;
			buffer.consume(crlfPos);
			break;
		}
		case CHUNK_DATA:
		{
			size_t available = buffer.readable();
			size_t toRead = std::min(available, _expectedBodySize);

			if (toRead > 0)
			{
				std::string chunkData;
				buffer.readBuffer(chunkData, toRead);
				_rawBody.writeBuffer(chunkData.c_str(), chunkData.length());
				_expectedBodySize -= toRead;
			}

			if (_expectedBodySize == 0)
			{
				_chunkState = CHUNK_TRAILERS;
			}
			break;
		}
		case CHUNK_TRAILERS:
		{
			std::string line;
			buffer.peekBuffer(line, buffer.readable());
			
			size_t crlfPos = line.find("\r\n");
			if (crlfPos == std::string::npos)
			{
				crlfPos = line.find('\n');
				if (crlfPos == std::string::npos)
				{
					// Need more data
					return BODY_PARSING;
				}
				crlfPos += 1; // Include the \n
			}
			else
			{
				crlfPos += 2; // Include the \r\n
			}

			buffer.consume(crlfPos);
			_chunkState = CHUNK_SIZE;
			break;
		}
		case CHUNK_COMPLETE:
		{
			return BODY_PARSING_COMPLETE;
		}
		}
	}

	return BODY_PARSING;
}

HttpBody::BodyState HttpBody::_parseContentLengthBody(RingBuffer &buffer, HttpResponse &response)
{
	(void)response; // TODO: Use response for error handling
	size_t available = buffer.readable();
	size_t needed = _expectedBodySize - _rawBody.readable();

	if (needed == 0)
	{
		return BODY_PARSING_COMPLETE;
	}

	size_t toRead = std::min(available, needed);
	if (toRead > 0)
	{
		std::string bodyData;
		buffer.readBuffer(bodyData, toRead);
		_rawBody.writeBuffer(bodyData.c_str(), bodyData.length());
	}

	if (_rawBody.readable() >= _expectedBodySize)
	{
		return BODY_PARSING_COMPLETE;
	}

	return BODY_PARSING;
}

size_t HttpBody::_parseHexSize(const std::string &hexStr) const
{
	if (hexStr.empty())
		return 0;

	// Remove any whitespace
	std::string trimmed = hexStr.substr(hexStr.find_first_not_of(" \t\r\n"), 
		hexStr.find_last_not_of(" \t\r\n") - hexStr.find_first_not_of(" \t\r\n") + 1);
	
	// Check if it's a valid hex string
	for (size_t i = 0; i < trimmed.length(); ++i)
	{
		if (!std::isxdigit(trimmed[i]))
		{
			Logger::log(Logger::ERROR, "Invalid hex chunk size: " + trimmed);
			return 0;
		}
	}

	char *endPtr;
	unsigned long size = std::strtoul(trimmed.c_str(), &endPtr, 16);
	if (*endPtr != '\0')
	{
		Logger::log(Logger::ERROR, "Invalid hex chunk size: " + trimmed);
		return 0;
	}

	return static_cast<size_t>(size);
}

/*
** --------------------------------- ACCESSORS --------------------------------
*/

HttpBody::BodyState HttpBody::getBodyState() const
{
	return _bodyState;
}

std::string HttpBody::getRawBody() const
{
	std::string body;
	_rawBody.peekBuffer(body, _rawBody.readable());
	return body;
}

size_t HttpBody::getBodySize() const
{
	return _rawBody.readable();
}

size_t HttpBody::getBodyBytesRead() const
{
	return _rawBody.readable();
}

bool HttpBody::getIsChunked() const
{
	return _bodyType == BODY_TYPE_CHUNKED;
}

bool HttpBody::getIsUsingTempFile() const
{
	return _isUsingTempFile;
}

std::string HttpBody::getTempFilePath() const
{
	if (_isUsingTempFile)
	{
		return _tempFile.getFilePath();
	}
	return "";
}

FileDescriptor &HttpBody::getTempFd()
{
	return _tempFile.getFd();
}

/*
** --------------------------------- MUTATORS --------------------------------
*/

void HttpBody::setBodyState(BodyState bodyState)
{
	_bodyState = bodyState;
}

void HttpBody::setBodyType(BodyType bodyType)
{
	_bodyType = bodyType;
}

void HttpBody::setExpectedBodySize(size_t expectedBodySize)
{
	_expectedBodySize = expectedBodySize;
}

void HttpBody::setBodySize(size_t bodySize)
{
	// This method is not used in the current implementation
	(void)bodySize;
}

void HttpBody::setIsUsingTempFile(bool isUsingTempFile)
{
	_isUsingTempFile = isUsingTempFile;
}

void HttpBody::setTempFilePath(const std::string &tempFilePath)
{
	_tempFile.setFilePath(tempFilePath);
}

void HttpBody::setTempFd(const FileDescriptor &tempFd)
{
	_tempFile.setFd(tempFd);
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpBody::reset()
{
	_bodyState = BODY_PARSING;
	_bodyType = BODY_TYPE_NO_BODY;
	_chunkState = CHUNK_SIZE;
	_expectedBodySize = 0;
	_chunkedBuffer.clear();
	_rawBody.clear();
	_tempChunkedFile.reset();
	_tempFile.reset();
	_isUsingTempFile = false;
=======
#include "../../includes/HTTP/HttpBody.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/HTTP/Constants.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
#include <algorithm>
#include <cstdlib>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

// TODO: Prevent temp file generation if not needed
HttpBody::HttpBody()
{
	_bodyState = BODY_PARSING;
	_bodyType = BODY_TYPE_NO_BODY;
	_chunkState = CHUNK_SIZE;
	_expectedBodySize = 0;
	_tempFile = FileManager();
	_isUsingTempFile = false;
}

HttpBody::HttpBody(HttpBody const &src)
{
	*this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

HttpBody::~HttpBody()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

HttpBody &HttpBody::operator=(HttpBody const &rhs)
{
	if (this != &rhs)
	{
		_bodyState = rhs._bodyState;
		_bodyType = rhs._bodyType;
		_chunkState = rhs._chunkState;
		_expectedBodySize = rhs._expectedBodySize;
		_rawBody = rhs._rawBody;
		_tempFile = rhs._tempFile;
		_isUsingTempFile = rhs._isUsingTempFile;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpBody::parseBuffer(std::vector<char> &buffer, HttpResponse &response)
{
	if (_bodyType == BODY_TYPE_NO_BODY)
		_bodyState = BODY_PARSING_COMPLETE;
	else if (_bodyType == BODY_TYPE_CHUNKED)
		_bodyState = _parseChunkedBody(buffer, response);
	else if (_bodyType == BODY_TYPE_CONTENT_LENGTH)
		_bodyState = _parseContentLengthBody(buffer, response);
	else
		_bodyState = BODY_PARSING_ERROR;
}

HttpBody::BodyState HttpBody::_parseContentLengthBody(std::vector<char> &buffer, HttpResponse &response)
{
	if (!_isUsingTempFile)
	{
		ssize_t bytes_needed = _expectedBodySize - _rawBody.size();
		if (bytes_needed < 0)
		{
			Logger::log(Logger::ERROR, "Body size exceeds expected size");
			response.setStatus(400, "Bad Request");
			return BODY_PARSING_ERROR;
		}
		ssize_t bytes_to_copy = std::min(bytes_needed, static_cast<ssize_t>(buffer.size()));
		_rawBody.insert(_rawBody.end(), buffer.begin(), buffer.begin() + bytes_to_copy);
		buffer.erase(buffer.begin(), buffer.begin() + bytes_to_copy);
		_rawBodySize += bytes_to_copy;
		if (_rawBodySize > _expectedBodySize)
		{
			Logger::log(Logger::ERROR, "Body size exceeds expected size");
			response.setStatus(400, "Bad Request");
			return BODY_PARSING_ERROR;
		}
		else if (_rawBody.size() >= HTTP::MAX_BODY_BUFFER_SIZE) // Flush to temp file
		{
			_isUsingTempFile = true;
			_tempFile.append(_rawBody, _rawBody.begin(), _rawBody.end());
			_rawBody.clear();
		}
	}
	else
	{
		ssize_t bytes_needed = _expectedBodySize - _tempFile.getFileSize();
		if (bytes_needed < 0)
		{
			Logger::log(Logger::ERROR, "Body size exceeds expected size");
			response.setStatus(400, "Bad Request");
			return BODY_PARSING_ERROR;
		}
		else
		{
			ssize_t bytes_to_copy = std::min(bytes_needed, static_cast<ssize_t>(buffer.size()));
			_tempFile.append(buffer, buffer.begin(), buffer.begin() + bytes_to_copy);
			buffer.erase(buffer.begin(), buffer.begin() + bytes_to_copy);
			_rawBodySize += bytes_to_copy;
		}
	}
	if (_rawBodySize == _expectedBodySize)
		return BODY_PARSING_COMPLETE;
	else
		return BODY_PARSING;
}

HttpBody::BodyState HttpBody::_parseChunkedBody(std::vector<char> &buffer, HttpResponse &response)
{
	switch (_chunkState)
	{
	case CHUNK_SIZE:
	{
		// Chunked size line validation
		std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), HTTP::CRLF, HTTP::CRLF + 2);
		if (it == buffer.end())
		{
			if (buffer.size() > 18) // Limit hex number size to 16 characters (8 bytes) + 2 for \r\n
			{
				Logger::log(Logger::ERROR, "Chunked size line too long");
				response.setStatus(400, "Bad Request");
				return BODY_PARSING_ERROR;
			}
			return BODY_PARSING;
		}
		// Extract size line
		std::string sizeLine(buffer.begin(), it);
		buffer.erase(buffer.begin(), it + 2);
		if (sizeLine.empty())
		{
			Logger::log(Logger::ERROR, "Empty chunk size line");
			response.setStatus(400, "Bad Request");
			return BODY_PARSING_ERROR;
		}
		else if (sizeLine.size() + 2 > 18) // 16 characters (8 bytes) + 2 for \r\n
		{
			Logger::log(Logger::ERROR, "Chunked size line too long");
			response.setStatus(400, "Bad Request");
			return BODY_PARSING_ERROR;
		}
		_expectedBodySize = _parseHexSize(sizeLine);
		if (_expectedBodySize == 0)
		{
			_chunkState = CHUNK_TRAILERS;
			return BODY_PARSING;
		}
		else if (_expectedBodySize == -1)
		{
			_chunkState = CHUNK_ERROR;
			Logger::log(Logger::ERROR, "Invalid chunk size: " + sizeLine);
			response.setStatus(400, "Bad Request");
			return BODY_PARSING_ERROR;
		}
		_chunkState = CHUNK_DATA;
		return BODY_PARSING;
	}
	case CHUNK_DATA: // Use a modified form of content length body parsing
	{
		// Abit simpler we find if current buffer has CRLF
		std::vector<char>::iterator extractableBytes =
			std::search(buffer.begin(), buffer.end(), HTTP::CRLF, HTTP::CRLF + 2);
		// Subtract the size of the extracted bytes from the expected body size
		_expectedBodySize -= extractableBytes - buffer.begin();
		// Add the size of the extracted bytes to the raw body size
		_rawBodySize += extractableBytes - buffer.begin();
		if (_expectedBodySize < 0)
		{
			Logger::log(Logger::ERROR, "Body size exceeds expected size");
			response.setStatus(400, "Bad Request");
			return BODY_PARSING_ERROR;
		}

		if (!_isUsingTempFile)
		{
			_rawBody.insert(_rawBody.end(), buffer.begin(), extractableBytes);
			if (_rawBodySize >= HTTP::MAX_BODY_BUFFER_SIZE)
			{
				_isUsingTempFile = true;
				_tempFile.append(_rawBody, _rawBody.begin(), _rawBody.end());
				_rawBody.clear();
			}
		}
		else
		{
			_tempFile.append(buffer, buffer.begin(), extractableBytes);
		}
		if (extractableBytes == buffer.end())
			buffer.erase(buffer.begin(), extractableBytes);
		else
			buffer.erase(buffer.begin(), extractableBytes + 2);
		if (_expectedBodySize == 0)
			_chunkState = CHUNK_SIZE;
		else
			_chunkState = CHUNK_DATA;
		return BODY_PARSING;
	}
	case CHUNK_TRAILERS: // TODO: Instead of discarding the trailers as a bonus we could parse them
	{
		std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), HTTP::CRLF, HTTP::CRLF + 2);
		_rawBodySize += it - buffer.begin();
		if (it == buffer.end())
		{
			if (buffer.size() > HTTP::MAX_HEADERS_LINE_SIZE)
			{
				Logger::log(Logger::ERROR, "Chunked trailers line too long");
				response.setStatus(400, "Bad Request");
				return BODY_PARSING_ERROR;
			}
			return BODY_PARSING;
		}
		else if (it == buffer.begin()) // If its /r/n then chunks are complete
		{
			buffer.erase(buffer.begin(), it + 2);
			_chunkState = CHUNK_COMPLETE;
			return BODY_PARSING_COMPLETE;
		}
		else if (it - buffer.begin() > HTTP::MAX_HEADERS_LINE_SIZE)
		{
			Logger::log(Logger::ERROR, "Chunked trailers line too long");
			response.setStatus(400, "Bad Request");
			return BODY_PARSING_ERROR;
		}
		buffer.erase(buffer.begin(), it + 2); // Clear the buffer up to the CRLF
		return BODY_PARSING;
	}
	case CHUNK_COMPLETE:
	{
		return BODY_PARSING_COMPLETE;
	}
	case CHUNK_ERROR:
	{
		return BODY_PARSING_ERROR;
	}
	}
	return BODY_PARSING;
}

ssize_t HttpBody::_parseHexSize(const std::string &hexStr) const
{
	char *endPtr;
	ssize_t size = std::strtoul(hexStr.c_str(), &endPtr, 16);
	if (*endPtr != '\0')
	{
		Logger::log(Logger::ERROR, "Invalid hex chunk size: " + hexStr);
		return -1;
	}
	return size;
}

/*
** --------------------------------- ACCESSORS --------------------------------
*/

HttpBody::BodyState HttpBody::getBodyState() const
{
	return _bodyState;
}

HttpBody::BodyType HttpBody::getBodyType() const
{
	return _bodyType;
}

std::string HttpBody::getRawBody() const
{
	std::string body;
	if (_isUsingTempFile)
	{
		body = _tempFile.getFd().readFile();
	}
	else
	{
		body.assign(_rawBody.begin(), _rawBody.end());
	}
	return body;
}

size_t HttpBody::getBodySize() const
{
	if (_isUsingTempFile)
	{
		return _tempFile.getFileSize();
	}
	else
	{
		return _rawBody.size();
	}
}

size_t HttpBody::getRawBodySize() const
{
	return _rawBody.size();
}

bool HttpBody::getIsUsingTempFile()
{
	return _isUsingTempFile;
}

std::string HttpBody::getTempFilePath()
{
	if (_isUsingTempFile)
	{
		return _tempFile.getFilePath();
	}
	return "";
}

FileDescriptor &HttpBody::getTempFd()
{
	return _tempFile.getFd();
}

/*
** --------------------------------- MUTATORS --------------------------------
*/

void HttpBody::setBodyState(BodyState bodyState)
{
	_bodyState = bodyState;
}

void HttpBody::setBodyType(BodyType bodyType)
{
	_bodyType = bodyType;
}

void HttpBody::setExpectedBodySize(size_t expectedBodySize)
{
	_expectedBodySize = expectedBodySize;
}

void HttpBody::setIsUsingTempFile(bool isUsingTempFile)
{
	_isUsingTempFile = isUsingTempFile;
}

void HttpBody::setTempFilePath(const std::string &tempFilePath)
{
	_tempFile.setFilePath(tempFilePath);
}

void HttpBody::setTempFd(const FileDescriptor &tempFd)
{
	_tempFile.setFd(tempFd);
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpBody::reset()
{
	_bodyState = BODY_PARSING;
	_bodyType = BODY_TYPE_NO_BODY;
	_chunkState = CHUNK_SIZE;
	_expectedBodySize = 0;
	_rawBody.clear();
	_tempFile.reset();
	_isUsingTempFile = false;
>>>>>>> ConfigParserRefactor
}