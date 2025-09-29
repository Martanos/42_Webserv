#include "../../includes/HttpBody.hpp"
#include "../../includes/HttpResponse.hpp"
#include "../../includes/Logger.hpp"
#include <algorithm>
#include <cctype>
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
		_bodyState = _parseContentLengthBody(buffer);
	else
		_bodyState = BODY_PARSING_ERROR;
}

HttpBody::BodyState HttpBody::_parseContentLengthBody(std::vector<char> &buffer)
{
	if (!_isUsingTempFile)
	{
		ssize_t bytes_needed = _expectedBodySize - _rawBody.size();
		ssize_t bytes_available = buffer.size() - 2; // 2 for \r\n

		if (bytes_needed > 0 && bytes_available >= bytes_needed)
		{
			_rawBody.insert(_rawBody.end(), buffer.begin(), buffer.begin() + bytes_needed);
			buffer.erase(buffer.begin(), buffer.begin() + bytes_needed + 2);
			if (_rawBody.size() >= HTTP::MAX_BODY_SIZE) // Flush to temp file
			{
				_isUsingTempFile = true;
				_tempFile = FileManager();
				_tempFile.append(_rawBody, _rawBody.begin(), _rawBody.end());
				_rawBody.clear();
			}
			return BODY_PARSING;
		}
		else
			return BODY_PARSING_COMPLETE;
	}
	else
	{
		ssize_t bytes_needed = _expectedBodySize - _tempFile.getFileSize();
		ssize_t bytes_available = buffer.size() - 2; // 2 for \r\n

		if (bytes_needed > 0 && bytes_available >= bytes_needed)
		{
			_tempFile.append(buffer, buffer.begin(), buffer.begin() + bytes_needed);
			buffer.erase(buffer.begin(), buffer.begin() + bytes_needed + 2);
			return BODY_PARSING;
		}
		else
			return BODY_PARSING_COMPLETE;
	}
}

HttpBody::BodyState HttpBody::_parseChunkedBody(std::vector<char> &buffer, HttpResponse &response)
{
	const char *crlfPattern = "\r\n";
	while (buffer.size() > 0)
	{
		switch (_chunkState)
		{
		case CHUNK_SIZE:
		{
			// Chunked size line validation
			std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), crlfPattern, crlfPattern + 2);
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
			std::string sizeLine;
			sizeLine.assign(buffer.begin(), it);
			buffer.erase(buffer.begin(), it + 2);

			if (sizeLine.empty())
			{
				Logger::log(Logger::ERROR, "Empty chunk size line");
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
			break;
		}
		case CHUNK_DATA: // Use a modified form of content length body parsing
		{
			if (!_isUsingTempFile)
			{
				ssize_t bytes_needed = _expectedBodySize - _rawBody.size();
				ssize_t bytes_available = buffer.size() - 2; // 2 for \r\n

				if (bytes_needed > 0 && bytes_available >= bytes_needed)
				{
					_rawBody.insert(_rawBody.end(), buffer.begin(), buffer.begin() + bytes_needed);
					buffer.erase(buffer.begin(), buffer.begin() + bytes_needed + 2); // 2 for \r\n
					if (_rawBody.size() >= HTTP::MAX_BODY_SIZE)						 // Flush to temp file
					{
						_isUsingTempFile = true;
						_tempFile = FileManager();
						_tempFile.append(_rawBody, _rawBody.begin(), _rawBody.end());
						_rawBody.clear();
					}
					_chunkState = CHUNK_SIZE;
				}
				return BODY_PARSING;
			}
			else
			{
				ssize_t bytes_needed = _expectedBodySize - _tempFile.getFileSize();
				ssize_t bytes_available = buffer.size() - 2; // 2 for \r\n

				if (bytes_needed > 0 && bytes_available >= bytes_needed)
				{
					_tempFile.append(buffer, buffer.begin(), buffer.begin() + bytes_needed);
					buffer.erase(buffer.begin(), buffer.begin() + bytes_needed + 2); // 2 for \r\n
					_chunkState = CHUNK_SIZE;
				}
				return BODY_PARSING;
			}
			break;
		}

		case CHUNK_TRAILERS:
		{
			std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), crlfPattern, crlfPattern + 2);
			if (it == buffer.end())
			{
				if (buffer.size() * sizeof(char) > static_cast<size_t>(sysconf(_SC_PAGESIZE) * 4))
				{
					Logger::log(Logger::ERROR, "Chunked trailers line too long");
					response.setStatus(400, "Bad Request");
					return BODY_PARSING_ERROR;
				}
				return BODY_PARSING;
			}
			buffer.erase(buffer.begin(), it + 2);
			_chunkState = CHUNK_COMPLETE;
			return BODY_PARSING_COMPLETE;
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
			return -1;
		}
	}

	char *endPtr;
	ssize_t size = std::strtoul(trimmed.c_str(), &endPtr, 16);
	if (*endPtr != '\0')
	{
		Logger::log(Logger::ERROR, "Invalid hex chunk size: " + trimmed);
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

size_t HttpBody::getBodySize()
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

bool HttpBody::getIsChunked()
{
	return _bodyType == BODY_TYPE_CHUNKED;
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
}