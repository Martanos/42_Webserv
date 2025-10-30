#include "../../includes/HTTP/HttpBody.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/HTTP/HTTP.hpp"
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
	_rawBodySize = 0;
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
		_rawBodySize = rhs._rawBodySize;
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
	Logger::debug("HttpBody: parseBuffer called, body type: " + StrUtils::toString(_bodyType) +
				  ", buffer size: " + StrUtils::toString(buffer.size()));
	if (_bodyType == BODY_TYPE_NO_BODY)
	{
		Logger::debug("HttpBody: No body type, marking as complete");
		_bodyState = BODY_PARSING_COMPLETE;
	}
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
	_rawBodySize = 0;
}