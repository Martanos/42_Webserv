#include "../../includes/HTTP/HttpBody.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/HTTP/HTTP.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
#include <algorithm>
#include <cstdlib>
#include <sys/ucontext.h>

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
					  ", buffer size: " + StrUtils::toString(buffer.size()),
				  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	if (_bodyType == BODY_TYPE_NO_BODY)
	{
		Logger::debug("HttpBody: No body type, marking as complete", __FILE__, __LINE__, __PRETTY_FUNCTION__);
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
	Logger::debug(
		"HttpBody: parseContentLengthBody called, expected body size: " + StrUtils::toString(_expectedBodySize) +
			", is using temp file: " + StrUtils::toString(_isUsingTempFile),
		__FILE__, __LINE__, __PRETTY_FUNCTION__);
	if (!_isUsingTempFile)
	{
		ssize_t bytes_needed = _expectedBodySize - _rawBody.size();
		if (bytes_needed < 0)
		{
			Logger::debug("Body size exceeds expected size", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			response.setResponseDefaultBody(400, "Body size exceeds expected size", NULL, NULL,
											HttpResponse::FATAL_ERROR);
			return BODY_PARSING_ERROR;
		}
		ssize_t bytes_to_copy = std::min(bytes_needed, static_cast<ssize_t>(buffer.size()));
		_rawBody.insert(_rawBody.end(), buffer.begin(), buffer.begin() + bytes_to_copy);
		buffer.erase(buffer.begin(), buffer.begin() + bytes_to_copy);
		_rawBodySize += bytes_to_copy;
		if (_rawBodySize > _expectedBodySize)
		{
			Logger::debug("Body size exceeds expected size", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			response.setResponseDefaultBody(400, "Body size exceeds expected size", NULL, NULL,
											HttpResponse::FATAL_ERROR);
			return BODY_PARSING_ERROR;
		}
		else if (_rawBody.size() >= HTTP::DEFAULT_CLIENT_MAX_BODY_SIZE) // Flush to temp file
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
			Logger::debug("Body size exceeds expected size", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			response.setResponseDefaultBody(400, "Body size exceeds expected size", NULL, NULL,
											HttpResponse::FATAL_ERROR);
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
	while (!buffer.empty())
	{
		switch (_chunkState)
		{
		case CHUNK_SIZE:
		{
			Logger::debug("HttpBody: Chunk size state", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			// Chunked size line validation
			std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), HTTP::CRLF, HTTP::CRLF + 2);
			Logger::debug("HttpBody: Chunk size line search result: " + StrUtils::toString(it - buffer.begin()),
						  __FILE__, __LINE__, __PRETTY_FUNCTION__);
			if (it == buffer.end()) // If the CRLF is not found, we need more data
			{
				if (buffer.size() > 18) // Limit hex number size to 16 characters (8 bytes) + 2 for \r\n
				{
					Logger::debug("Chunked transfer encoding size string exceeded limit", __FILE__, __LINE__,
								  __PRETTY_FUNCTION__);
					response.setResponseDefaultBody(400, "Chunked transfer encoding size string exceeded limit", NULL,
													NULL, HttpResponse::FATAL_ERROR);
					return BODY_PARSING_ERROR;
				}
				return BODY_PARSING;
			}
			// Extract size line
			std::string sizeLine(buffer.begin(), it);
			buffer.erase(buffer.begin(), it + 2);
			if (sizeLine.empty())
			{
				Logger::debug("Empty chunk size line", __FILE__, __LINE__, __PRETTY_FUNCTION__);
				response.setResponseDefaultBody(400, "Empty chunk size line", NULL, NULL, HttpResponse::FATAL_ERROR);
				return BODY_PARSING_ERROR;
			}
			else if (sizeLine.size() + 2 > 18) // 16 characters (8 bytes) + 2 for \r\n
			{
				Logger::debug("Chunked transfer encoding size string exceeded limit", __FILE__, __LINE__,
							  __PRETTY_FUNCTION__);
				response.setResponseDefaultBody(400, "Chunked transfer encoding size string exceeded limit", NULL, NULL,
												HttpResponse::FATAL_ERROR);
				return BODY_PARSING_ERROR;
			}
			_expectedBodySize = _parseHexSize(sizeLine);
			_rawBodySize += _expectedBodySize; // Add the expected body size to the raw body size
			if (_expectedBodySize == 0)
			{
				Logger::debug("Chunked transfer encoding size is 0, switching to trailers state", __FILE__, __LINE__,
							  __PRETTY_FUNCTION__);
				_chunkState = CHUNK_TRAILERS;
				break;
			}
			else if (_expectedBodySize == -1)
			{
				_chunkState = CHUNK_ERROR;
				Logger::debug("Invalid chunk size: " + sizeLine, __FILE__, __LINE__, __PRETTY_FUNCTION__);
				response.setResponseDefaultBody(400, "Invalid chunk size: " + sizeLine, NULL, NULL,
												HttpResponse::FATAL_ERROR);
				return BODY_PARSING_ERROR;
			}
			_chunkState = CHUNK_DATA;
			break;
		}
		case CHUNK_DATA: // Use a modified form of content length body parsing
		{
			// We search for the CRLF in the buffer to find the end of the chunk data
			std::vector<char>::iterator extractableBytes =
				std::search(buffer.begin(), buffer.end(), HTTP::CRLF, HTTP::CRLF + 2);
			// If the CRLF is not found, we need more data
			if (extractableBytes == buffer.end())
			{
				if (buffer.size() >
					static_cast<size_t>(_expectedBodySize)) // If the buffer size is greater than the expected body size
															// a fatal error is returned
				{
					Logger::debug("Chunked transfer encoding body size exceeds expected size", __FILE__, __LINE__,
								  __PRETTY_FUNCTION__);
					response.setResponseDefaultBody(400, "Chunked transfer encoding body size exceeds expected size",
													NULL, NULL, HttpResponse::FATAL_ERROR);
					return BODY_PARSING_ERROR;
				}
				return BODY_PARSING;
			}
			if (!_isUsingTempFile)
			{
				_rawBody.insert(_rawBody.end(), buffer.begin(), extractableBytes);
				if (_rawBodySize >= HTTP::DEFAULT_CLIENT_MAX_BODY_SIZE)
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
			buffer.erase(buffer.begin(), extractableBytes + 2); // Clear the buffer up to the CRLF
			_chunkState = CHUNK_SIZE;
			break;
		}
		case CHUNK_TRAILERS: // TODO: Instead of discarding the trailers as a bonus we could parse them
		{
			std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), HTTP::CRLF, HTTP::CRLF + 2);
			_rawBodySize += it - buffer.begin();
			if (it == buffer.end())
			{
				if (buffer.size() > HTTP::DEFAULT_CLIENT_MAX_HEADERS_SIZE)
				{
					Logger::debug("Chunked transfer encoding trailers line too long", __FILE__, __LINE__,
								  __PRETTY_FUNCTION__);
					response.setResponseDefaultBody(400, "Chunked transfer encoding trailers line too long", NULL, NULL,
													HttpResponse::FATAL_ERROR);
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
			else if (it - buffer.begin() > HTTP::DEFAULT_CLIENT_MAX_HEADERS_SIZE)
			{
				Logger::debug("Chunked transfer encoding trailers line too long", __FILE__, __LINE__,
							  __PRETTY_FUNCTION__);
				response.setResponseDefaultBody(400, "Chunked transfer encoding trailers line too long", NULL, NULL,
												HttpResponse::FATAL_ERROR);
				return BODY_PARSING_ERROR;
			}
			buffer.erase(buffer.begin(), it + 2); // Clear the buffer up to the CRLF
			break;
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

const FileDescriptor &HttpBody::getTempFd() const
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
