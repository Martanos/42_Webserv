#include "../../includes/HttpBody.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpBody::HttpBody()
{
	_bodyState = BODY_PARSING;
	_bodyType = BODY_TYPE_NO_BODY;
	_chunkState = CHUNK_SIZE;
	_expectedBodySize = 0;
	_rawBody = RingBuffer(HTTP::MAX_BODY_SIZE);
	_chunkedBuffer = RingBuffer(HTTP::MAX_BODY_SIZE);
	_tempFile = FileManager();
	_isUsingTempFile = false;
}

HttpBody::HttpBody(const HttpBody &src)
{
	*this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

// If the file isn't moved that means it wasn't used and hence needs to be
// cleaned up
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
		_expectedBodySize = rhs._expectedBodySize;
		_bodyType = rhs._bodyType;
		_chunkState = rhs._chunkState;
		_tempFile = rhs._tempFile;
		_rawBody = rhs._rawBody;
		_chunkedBuffer = rhs._chunkedBuffer;
		_isUsingTempFile = rhs._isUsingTempFile;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpBody::reset()
{
	_bodyState = BODY_PARSING;
	_expectedBodySize = 0;
	_bodyType = BODY_TYPE_NO_BODY;
	_chunkState = CHUNK_SIZE;
	_rawBody.reset();
	_tempFile.reset();
	_isUsingTempFile = false;
}

int HttpBody::parseBuffer(RingBuffer &buffer, HttpResponse &response)
{
	switch (_bodyState)
	{
	case BODY_PARSING:
	{
		switch (_bodyType)
		{
		case BODY_TYPE_CHUNKED:
		{
			// 1. Transfer incoming buffer data to either a buffer or a temp
			// file
			_bodyState = _parseChunkedBody(buffer, response);
			break;
		}
		case BODY_TYPE_CONTENT_LENGTH:
			_bodyState = _parseContentLengthBody(buffer, response);
			break;
		case BODY_TYPE_NO_BODY:
			_bodyState = BODY_PARSING_COMPLETE;
			break;
		default:
			_bodyState = BODY_PARSING_ERROR;
			break;
		}
		break;
	}
	case BODY_PARSING_COMPLETE:
		return _bodyState;
	}
	return _bodyState;
}

HttpBody::BodyState HttpBody::_parseChunkedBody(RingBuffer &buffer, HttpResponse &response)
{
	// First attempt to transfer buffer to either a buffer or a temp file
	if (_chunkedBuffer.writable() >= buffer.readable() && !_isUsingTempFile)
	{
		_chunkedBuffer.transferFrom(buffer, buffer.readable());
	}
	else
	{
		if (_chunkedBuffer.readable() > 0)
		{
			_chunkedBuffer.flushToFile(_tempChunkedFile.getFd());
			_chunkedBuffer.clear();
		}
		buffer.flushToFile(_tempChunkedFile.getFd());
		_isUsingTempFile = true;
	}
	switch (_chunkState)
	{
	case CHUNK_SIZE:
	{
		// Look for CLRF to get chunk size line
		if (_isUsingTempFile)
		{
			size_t newlinePos = _tempChunkedFile.getFd().contains("\r\n", 2);
			if (newlinePos == _tempChunkedFile.getFd().capacity()) // No CLRF found transfer to local
																   // buffer and wait for more data
			{
				return BODY_PARSING;
			}
		}
		else
		{
			size_t newlinePos = _chunkedBuffer.contains("\r\n", 2);
			if (newlinePos == _chunkedBuffer.capacity()) // No CLRF found transfer to local
														 // buffer and wait for more data
			{
				return BODY_PARSING;
			}
		}
		// Translate chunk size line to raw body
		std::string chunkSizeLine;
		_chunkedBuffer.readBuffer(chunkSizeLine,
								  newlinePos + 2);			  // Include the CLRF to clear CLRF from the buffer
		chunkSizeLine += chunkSizeLine.substr(0, newlinePos); // Exclude the CLRF to ignore it

		size_t semicolonPos = chunkSizeLine.find(';');
		std::string hexSize = chunkSizeLine.substr(0, semicolonPos);

		size_t chunkSize = _parseHexSize(hexSize);
		if (chunkSize == -1) // Invalid hex
		{
			Logger::log(Logger::ERROR, "Invalid chunk size: " + hexSize);
			response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
			response.setStatusMessage("Bad Request");
			response.setBody("Bad Request");
			response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
			response.setHeader("Connection", "close");
			return BODY_PARSING_ERROR;
		}
		if (chunkSize == 0) // No data to read
		{
			_chunkState = CHUNK_TRAILERS;
		}
		else
		{
			_expectedBodySize = chunkSize + 2; // +2 for the CLRF
			_chunkedBuffer.reserve(chunkSize);
			_chunkState = CHUNK_DATA;
		}
		break;
	}
	case CHUNK_DATA:
	{
		// Extract data from chunked buffer and transfer to raw body
		std::string chunkData;
		_chunkedBuffer.readBuffer(chunkData,
								  _expectedBodySize - 2); // -2 for the CLRF
				if ( chunkData == ""
		_rawBody.writeBuffer(chunkData.c_str(), chunkData.size());
		_chunkedBuffer.consume(_expectedBodySize - 2);
		_chunkState = CHUNK_TRAILERS;
		break;
	}
	case CHUNK_TRAILERS:
	{
	}
	break;
		_rawBody.writeBuffer(chunkData.c_str(), chunkData.size());
		if (_rawBody.readable() == _expectedBodySize)
		{
			_bodyState = BODY_PARSING_COMPLETE;
			return _bodyState;
		}
		break;
	}
}

HttpBody::BodyState HttpBody::_parseContentLengthBody(RingBuffer &buffer, HttpResponse &response)
{
	// Check if the body is meant to be empty
	if (_expectedBodySize == 0)
	{
		_bodyState = BODY_PARSING_COMPLETE;
		return;
	}
	// Check that if appended the buffer + digested data is within expected size
	if (_isUsingTempFile)
	{
		if (_tempFile.getFileSize() + buffer.readable() > _expectedBodySize)
		{
			// Received more data than expected - protocol violation
			_bodyState = BODY_PARSING_ERROR;
		}
	}
	else
	{
		if (_rawBody.readable() + buffer.readable() > _expectedBodySize)
		{
			_bodyState = BODY_PARSING_ERROR;
		}
	}
	if (_bodyState == BODY_PARSING_ERROR)
	{
		Logger::log(Logger::ERROR, "Received more body data than Content-Length specified");
		response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
		response.setStatusMessage("Bad Request");
		response.setBody("Bad Request");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		return _bodyState;
	}

	// If not attempt to write body to buffer
	if (_rawBody.writable() >= buffer.readable() && !_isUsingTempFile)
	{
		_rawBody.writeBuffer(buffer, buffer.readable());
	}
	else // if internal buffer has reached its limit, flush to temp file
	{

		// If there is data in the buffer, flush it to the temp file
		if (_rawBody.readable() > 0)
		{
			_rawBody.flushToFile(_tempFile.getFd());
		}
		// Write remaining buffer to temp file
		buffer.flushToFile(_tempFile.getFd());
		_isUsingTempFile = true;
	}

	// Check if data contains entire body
	if (_isUsingTempFile)
	{
		if (_tempFile.getFileSize() == _expectedBodySize)
		{
			_bodyState = BODY_PARSING_COMPLETE;
			return _bodyState;
		}
	}
	else
	{
		if (_rawBody.readable() == _expectedBodySize)
		{
			_bodyState = BODY_PARSING_COMPLETE;
			return _bodyState;
		}
	}
	return _bodyState;
}

/*
** --------------------------------- DECODING METHODS
*----------------------------------
*/

// Parse hexadecimal chunk size
size_t HttpBody::_parseHexSize(const std::string &hexStr) const
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

// TODO: 64bit decoding

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

HttpBody::BodyState HttpBody::getBodyState() const
{
	return _bodyState;
}

bool HttpBody::getIsChunked() const
{
	return _bodyType == BODY_TYPE_CHUNKED;
}

bool HttpBody::getIsUsingTempFile() const
{
	return _tempFile.getIsUsingTempFile();
}

/*
** --------------------------------- MUTATORS ---------------------------------
*/

std::string HttpBody::getTempFilePath() const
{
	return _tempFile.getFilePath();
}

FileDescriptor &HttpBody::getTempFd()
{
	return _tempFile.getFd();
}

void HttpBody::setBodyState(BodyState bodyState)
{
	_bodyState = bodyState;
}

void HttpBody::setExpectedBodySize(size_t expectedBodySize)
{
	_expectedBodySize = expectedBodySize;
}

void HttpBody::setIsUsingTempFile(bool isUsingTempFile)
{
	_tempFile.setIsUsingTempFile(isUsingTempFile);
}

void HttpBody::setTempFilePath(const std::string &tempFilePath)
{
	_tempFile.setFilePath(tempFilePath);
}

void HttpBody::setTempFd(const FileDescriptor &tempFd)
{
	_tempFile.setFd(tempFd);
}

void HttpBody::setBodyType(BodyType bodyType)
{
	_bodyType = bodyType;
}

/* ************************************************************************** */
