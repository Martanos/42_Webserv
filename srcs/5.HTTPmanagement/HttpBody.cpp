#include "HttpBody.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpBody::HttpBody()
{
	_bodyState = BODY_PARSING;
	_expectedBodySize = 0;
	_bodyBytesRead = 0;
	_isChunked = false;
	_isUsingTempFile = false;
	_tempFilePath = "";
	_tempFd = FileDescriptor(-1);
}

HttpBody::HttpBody(const HttpBody &src)
{
	if (this != &src)
	{
		_bodyState = src._bodyState;
		_expectedBodySize = src._expectedBodySize;
		_bodyBytesRead = src._bodyBytesRead;
		_isChunked = src._isChunked;
		_isUsingTempFile = src._isUsingTempFile;
		_tempFilePath = src._tempFilePath;
		_tempFd = src._tempFd;
	}
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

// If the file isn't moved that means it wasn't used and hence needs to be cleaned up
HttpBody::~HttpBody()
{
	if (_isUsingTempFile && _bodyState != BODY_PARSING_COMPLETE)
	{
		_cleanupTempFile();
	}
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
		_bodyBytesRead = rhs._bodyBytesRead;
		_isChunked = rhs._isChunked;
		_isUsingTempFile = rhs._isUsingTempFile;
		_tempFilePath = rhs._tempFilePath;
		_tempFd = rhs._tempFd;
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
	_isChunked = false;
	_isUsingTempFile = false;
	_tempFilePath = "";
	_tempFd = FileDescriptor(-1);
}

int HttpBody::parseBuffer(std::string &buffer, HttpResponse &response)
{

	switch (_bodyState)
	{
	case BODY_PARSING:
		_parseBody(buffer, response);
		break;
	case BODY_PARSING_COMPLETE:
		break;
	}
}

/*
** --------------------------------- FILE MANAGEMENT METHODS ----------------------------------
*/

std::string HttpBody::_generateTempFilePath()
{
	std::time_t now = std::time(0);
	char buf[80];
	std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", std::localtime(&now));
	// Check that the generated name doesn't already exist
	if (access((std::string(HTTP::TEMP_FILE_TEMPLATE) + std::string(buf)).c_str(), F_OK) == 0)
	{
		return std::string(HTTP::TEMP_FILE_TEMPLATE) + std::string(buf);
	}
	// If not try again
	return _generateTempFilePath();
}

// TODO: Impleemnt more safe guards
void HttpBody::_switchToTempFile()
{
	if (_isUsingTempFile)
	{
		return; // Already using temp file
	}
	_tempFilePath = _generateTempFilePath();
	_tempFd = FileDescriptor(open(_tempFilePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644));
	_isUsingTempFile = true;
}

void HttpBody::_cleanupTempFile()
{
	if (_isUsingTempFile && !_tempFilePath.empty())
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
		_isUsingTempFile = false;
	}
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

std::string HttpBody::getRawBody() const
{
	return _rawBody;
}

size_t HttpBody::getBodySize() const
{
	return _bodySize;
}

size_t HttpBody::getBodyBytesRead() const
{
	return _bodyBytesRead;
}

bool HttpBody::getIsChunked() const
{
	return _isChunked;
}

bool HttpBody::getIsUsingTempFile() const
{
	return _isUsingTempFile;
}

/*
** --------------------------------- MUTATORS ---------------------------------
*/

std::string HttpBody::getTempFilePath() const
{
	return _tempFilePath;
}

FileDescriptor HttpBody::getTempFd() const
{
	return _tempFd;
}

void HttpBody::setBodyState(BodyState bodyState)
{
	_bodyState = bodyState;
}

void HttpBody::setRawBody(const std::string &rawBody)
{
	_rawBody = rawBody;
}

void HttpBody::setBodySize(size_t bodySize)
{
	_bodySize = bodySize;
}

void HttpBody::setBodyBytesRead(size_t bodyBytesRead)
{
	_bodyBytesRead = bodyBytesRead;
}

void HttpBody::setIsChunked(bool isChunked)
{
	_isChunked = isChunked;
}

void HttpBody::setIsUsingTempFile(bool isUsingTempFile)
{
	_isUsingTempFile = isUsingTempFile;
}

void HttpBody::setTempFilePath(const std::string &tempFilePath)
{
	_tempFilePath = tempFilePath;
}

void HttpBody::setTempFd(const FileDescriptor &tempFd)
{
	_tempFd = tempFd;
}

/* ************************************************************************** */
