#include "../../includes/Http/HttpBody.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Http/HTTP.hpp"
#include "../../includes/Http/HttpResponse.hpp"
#include "../../includes/Utils/StrUtils.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ucontext.h>
#include <unistd.h>

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

void HttpBody::parseBuffer(std::vector<char> &buffer, const Location &location, HttpResponse &response)
{
	if (_bodyType == BODY_TYPE_NO_BODY)
	{
		_bodyState = BODY_PARSING_COMPLETE;
	}
	else if (_bodyType == BODY_TYPE_CHUNKED)
		_bodyState = _parseChunkedBody(buffer, location, response);
	else if (_bodyType == BODY_TYPE_CONTENT_LENGTH)
		_bodyState = _parseContentLengthBody(buffer, location, response);
	else
		_bodyState = BODY_PARSING_ERROR;
}

HttpBody::BodyState HttpBody::_parseContentLengthBody(std::vector<char> &buffer, const Location &location,
													  HttpResponse &response)
{
	// Check intially if expected body size exceeds client max body size
	if (_expectedBodySize > static_cast<ssize_t>(location.getClientMaxBodySize()))
	{
		response.setResponseDefaultBody(413, "Body size exceeds client max body size", &location,
										HttpResponse::FATAL_ERROR);
		Logger::error("HttpBody: Body size exceeds client max body size of " +
						  StrUtils::toString(location.getClientMaxBodySize()),
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
		return BODY_PARSING_ERROR;
	}
	// Read body data
	if (!_isUsingTempFile)
	{
		// Check if current bo
		ssize_t bytes_needed = _expectedBodySize - _rawBody.size();
		if (bytes_needed < 0)
		{
			response.setResponseDefaultBody(400, "Body size exceeds expected size", &location,
											HttpResponse::FATAL_ERROR);
			Logger::error("HttpBody: Body size exceeds expected size", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			return BODY_PARSING_ERROR;
		}
		ssize_t bytes_to_copy = std::min(bytes_needed, static_cast<ssize_t>(buffer.size()));
		_rawBody.insert(_rawBody.end(), buffer.begin(), buffer.begin() + bytes_to_copy);
		buffer.erase(buffer.begin(), buffer.begin() + bytes_to_copy);
		_rawBodySize += bytes_to_copy;
		if (_rawBodySize > _expectedBodySize)
		{
			response.setResponseDefaultBody(400, "Body size exceeds expected size given by headers", &location,
											HttpResponse::FATAL_ERROR);
			Logger::error("HttpBody: Body size exceeds expected size given by headers", __FILE__, __LINE__,
						  __PRETTY_FUNCTION__);
			return BODY_PARSING_ERROR;
		}
		else if (_rawBodySize > location.getClientMaxBodySize())
		{
			response.setResponseDefaultBody(413, "Body size exceeds client max body size set in configuration",
											&location, HttpResponse::FATAL_ERROR);
			Logger::error("HttpBody: Body size exceeds client max body size set in configuration", __FILE__, __LINE__,
						  __PRETTY_FUNCTION__);
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
			response.setResponseDefaultBody(400, "Body size exceeds expected size", &location,
											HttpResponse::FATAL_ERROR);
			Logger::error("HttpBody: Body size exceeds expected size", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			return BODY_PARSING_ERROR;
		}
		else
		{
			ssize_t bytes_to_copy = std::min(bytes_needed, static_cast<ssize_t>(buffer.size()));
			_tempFile.append(buffer, buffer.begin(), buffer.begin() + bytes_to_copy);
			buffer.erase(buffer.begin(), buffer.begin() + bytes_to_copy);
			_rawBodySize += bytes_to_copy;
		}
		if (_rawBodySize > location.getClientMaxBodySize())
		{
			response.setResponseDefaultBody(413, "Body size exceeds client max body size set in configuration",
											&location, HttpResponse::FATAL_ERROR);
			Logger::error("HttpBody: Body size exceeds client max body size set in configuration", __FILE__, __LINE__,
						  __PRETTY_FUNCTION__);
			return BODY_PARSING_ERROR;
		}
	}
	if (_rawBodySize == _expectedBodySize)
		return BODY_PARSING_COMPLETE;
	else
		return BODY_PARSING;
}

HttpBody::BodyState HttpBody::_parseChunkedBody(std::vector<char> &buffer, const Location &location,
												HttpResponse &response)
{
	while (!buffer.empty())
	{
		switch (_chunkState)
		{
		case CHUNK_SIZE:
		{
			// Chunked size line validation
			std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), HTTP::CRLF, HTTP::CRLF + 2);
			if (it == buffer.end()) // If the CRLF is not found, we need more data
			{
				if (buffer.size() > 18) // Limit hex number size to 16 characters (8 bytes) + 2 for \r\n
				{
					response.setResponseDefaultBody(400, "Chunked transfer encoding size string exceeded limit",
													&location, HttpResponse::FATAL_ERROR);
					Logger::error("HttpBody: Chunked transfer encoding size string exceeded limit", __FILE__, __LINE__,
								  __PRETTY_FUNCTION__);
					return BODY_PARSING_ERROR;
				}
				return BODY_PARSING;
			}
			// Extract size line
			std::string sizeLine(buffer.begin(), it);
			buffer.erase(buffer.begin(), it + 2);
			_rawBodySize += sizeLine.size() + 2; // Add size line and CRLF to raw body size
			if (sizeLine.empty())
			{
				response.setResponseDefaultBody(400, "Empty chunk size line", &location, HttpResponse::FATAL_ERROR);
				Logger::error("HttpBody: Empty chunk size line", __FILE__, __LINE__, __PRETTY_FUNCTION__);
				return BODY_PARSING_ERROR;
			}
			else if (sizeLine.size() + 2 > 18) // 16 characters (8 bytes) + 2 for \r\n
			{
				response.setResponseDefaultBody(400, "Chunked transfer encoding size string exceeded limit", &location,
												HttpResponse::FATAL_ERROR);
				Logger::error("HttpBody: Chunked transfer encoding size string exceeded limit", __FILE__, __LINE__,
							  __PRETTY_FUNCTION__);
				return BODY_PARSING_ERROR;
			}
			_expectedBodySize = _parseHexSize(sizeLine);
			_rawBodySize += _expectedBodySize; // Add the expected body size to the raw body size
			if (_rawBodySize > location.getClientMaxBodySize())
			{
				response.setResponseDefaultBody(413, "Body size exceeds client max body size set in configuration",
												&location, HttpResponse::FATAL_ERROR);
				Logger::error("HttpBody: Body size exceeds client max body size set in configuration " +
								  StrUtils::toString(_rawBodySize) + " > " +
								  StrUtils::toString(location.getClientMaxBodySize()),
							  __FILE__, __LINE__, __PRETTY_FUNCTION__);
				return BODY_PARSING_ERROR;
			}
			if (_expectedBodySize == 0)
			{
				_chunkState = CHUNK_TRAILERS;
				break;
			}
			else if (_expectedBodySize == -1)
			{
				_chunkState = CHUNK_ERROR;
				response.setResponseDefaultBody(400, "Invalid chunk size: " + sizeLine, &location,
												HttpResponse::FATAL_ERROR);
				Logger::error("HttpBody: Invalid chunk size: " + sizeLine, __FILE__, __LINE__, __PRETTY_FUNCTION__);
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
				if (buffer.size() > static_cast<size_t>(_expectedBodySize)) // If the buffer size is greater than the
																			// expected body size a fatal error is
																			// returned
				{
					response.setResponseDefaultBody(400, "Chunked transfer encoding body size exceeds expected size",
													&location, HttpResponse::FATAL_ERROR);
					Logger::error("HttpBody: Chunked transfer encoding body size exceeds expected size", __FILE__,
								  __LINE__, __PRETTY_FUNCTION__);
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
			buffer.erase(buffer.begin(),
						 extractableBytes + 2); // Clear the buffer up to the CRLF
			_chunkState = CHUNK_SIZE;
			break;
		}
		case CHUNK_TRAILERS: // TODO: Instead of discarding the trailers as a bonus
							 // we could parse them
		{
			std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), HTTP::CRLF, HTTP::CRLF + 2);
			if (it == buffer.end())
			{
				if (buffer.size() > HTTP::DEFAULT_CLIENT_MAX_HEADERS_SIZE)
				{
					response.setResponseDefaultBody(400, "Chunked transfer encoding trailers line too long", &location,
													HttpResponse::FATAL_ERROR);
					Logger::error("HttpBody: Chunked transfer encoding trailers line too long", __FILE__, __LINE__,
								  __PRETTY_FUNCTION__);
					return BODY_PARSING_ERROR;
				}
				return BODY_PARSING;
			}
			else if (it == buffer.begin()) // If its /r/n then chunks are complete
			{
				_rawBodySize += it - buffer.begin() + 2; // Add trailer line and CRLF to raw body size
				if (_rawBodySize > location.getClientMaxBodySize())
				{
					response.setResponseDefaultBody(413, "Body size exceeds client max body size set in configuration",
													&location, HttpResponse::FATAL_ERROR);
					Logger::error("HttpBody: Body size exceeds client max body size set in configuration: " +
									  StrUtils::toString(_rawBodySize) + " > " +
									  StrUtils::toString(location.getClientMaxBodySize()),
								  __FILE__, __LINE__, __PRETTY_FUNCTION__);
					return BODY_PARSING_ERROR;
				}
				buffer.erase(buffer.begin(), it + 2);
				_chunkState = CHUNK_COMPLETE;
				return BODY_PARSING_COMPLETE;
			}
			else if (it - buffer.begin() > HTTP::DEFAULT_CLIENT_MAX_HEADERS_SIZE)
			{
				response.setResponseDefaultBody(400, "Chunked transfer encoding trailers line too long", NULL,
												HttpResponse::FATAL_ERROR);
				Logger::error("HttpBody: Chunked transfer encoding trailers line too long", __FILE__, __LINE__,
							  __PRETTY_FUNCTION__);
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
		Logger::error("Invalid hex size string: " + hexStr, __FILE__, __LINE__, __PRETTY_FUNCTION__);
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

// TODO: Split this into getRawBodyFromMemory() and getRawBodyFromTempFile()
std::string HttpBody::getRawBody() const
{
	std::string body;
	if (_isUsingTempFile)
	{
		// Sync the temp file to ensure all data is written to disk before reading
		// This is necessary because we're opening a new fd to read the data
		const FileDescriptor &tempFd = _tempFile.getFd();
		if (tempFd.getFd() != -1)
		{
			if (fsync(tempFd.getFd()) == -1)
			{
				Logger::warning("HttpBody: Failed to fsync temp file before reading: " + std::string(strerror(errno)),
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}

		// Open the temp file fresh and read all content
		std::string tempPath = _tempFile.getFilePath();
		if (tempPath.empty())
		{
			Logger::error("HttpBody: Temp file path is empty", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			return body;
		}
		int fd = open(tempPath.c_str(), O_RDONLY);
		if (fd == -1)
		{
			Logger::error("HttpBody: Failed to open temp file for reading: " + std::string(strerror(errno)), __FILE__,
						  __LINE__, __PRETTY_FUNCTION__);
			return body;
		}
		// Get file size
		struct stat st;
		if (fstat(fd, &st) == -1)
		{
			Logger::error("HttpBody: Failed to stat temp file: " + std::string(strerror(errno)), __FILE__, __LINE__,
						  __PRETTY_FUNCTION__);
			close(fd);
			return body;
		}
		// Read entire file
		body.resize(st.st_size);
		ssize_t totalRead = 0;
		while (totalRead < st.st_size)
		{
			ssize_t bytesRead = read(fd, &body[totalRead], st.st_size - totalRead);
			if (bytesRead == -1)
			{
				if (errno == EINTR)
					continue;
				Logger::error("HttpBody: Failed to read temp file: " + std::string(strerror(errno)), __FILE__, __LINE__,
							  __PRETTY_FUNCTION__);
				close(fd);
				body.clear();
				return body;
			}
			if (bytesRead == 0)
				break;
			totalRead += bytesRead;
		}
		close(fd);
		body.resize(totalRead);
		Logger::debug("HttpBody: Read " + StrUtils::toString(totalRead) + " bytes from temp file " + tempPath, __FILE__,
					  __LINE__, __PRETTY_FUNCTION__);
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
	return _rawBodySize;
}

bool HttpBody::getIsUsingTempFile() const
{
	return _isUsingTempFile;
}

FileManager &HttpBody::getTempFileManager()
{
	return _tempFile;
}

const FileManager &HttpBody::getTempFileManager() const
{
	return _tempFile;
}

std::string HttpBody::getTempFilePath() const
{
	return _tempFile.getFilePath();
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
