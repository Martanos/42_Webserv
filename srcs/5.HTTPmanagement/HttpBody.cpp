#include "HttpBody.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpBody::HttpBody()
{
	_bodyState = BODY_PARSING;
	_bodyType = BODY_TYPE_NO_BODY;
	_chunkState = CHUNK_SIZE;
	_expectedBodySize = 0;
	_tempFile = FileManager();
}

HttpBody::HttpBody(const HttpBody &src)
{
	if (this != &src)
	{
		_bodyState = src._bodyState;
		_bodyType = src._bodyType;
		_chunkState = src._chunkState;
		_expectedBodySize = src._expectedBodySize;
		_tempFile = src._tempFile;
	}
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

// If the file isn't moved that means it wasn't used and hence needs to be cleaned up
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
	_tempFile.reset();
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
			_parseChunkedBody(buffer, response);
			break;
		case BODY_TYPE_CONTENT_LENGTH:
			_parseContentLengthBody(buffer, response);
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
		break;
	}
	return _bodyState;
}

void HttpBody::_parseChunkedBody(RingBuffer &buffer, HttpResponse &response)
{
	switch (_chunkState)
	{
	case CHUNK_SIZE:
		break;
	case CHUNK_DATA:
		break;
	case CHUNK_TRAILERS:
		break;
	case CHUNK_COMPLETE:
		break;
	}
}

void HttpBody::_parseContentLengthBody(RingBuffer &buffer, HttpResponse &response)
{
	if (_expectedBodySize == 0)
	{
		_bodyState = BODY_PARSING_COMPLETE;
		return;
	}
	else if (_expectedBodySize >)
}

/*
** --------------------------------- DECODING METHODS ----------------------------------
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

FileDescriptor HttpBody::getTempFd() const
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
