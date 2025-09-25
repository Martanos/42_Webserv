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
}