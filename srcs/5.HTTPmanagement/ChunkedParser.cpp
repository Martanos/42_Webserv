#include "ChunkedParser.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ChunkedParser::ChunkedParser()
{
	_state = CHUNK_SIZE;
	_currentChunkSize = 0;
	_bytesRead = 0;
	_isComplete = false;
}

ChunkedParser::ChunkedParser(const ChunkedParser &src)
{
	throw std::runtime_error("ChunkedParser: Copy constructor not implemented");
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ChunkedParser::~ChunkedParser()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ChunkedParser &ChunkedParser::operator=(ChunkedParser const &rhs)
{
	(void)rhs;
	throw std::runtime_error("ChunkedParser: Copy assignment operator not implemented");
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

ChunkedParser::ChunkState ChunkedParser::processBuffer(const std::string &buffer)
{
	_chunkBuffer += buffer;

	while (_state != CHUNK_COMPLETE && _state != CHUNK_ERROR && !_chunkBuffer.empty())
	{
		switch (_state)
		{
		case CHUNK_SIZE:
		{
			size_t crlfPos = _chunkBuffer.find("\r\n");
			if (crlfPos == std::string::npos)
			{
				// Need more data for complete chunk size line
				return _state;
			}

			std::string chunkSizeLine = _chunkBuffer.substr(0, crlfPos);
			_chunkBuffer.erase(0, crlfPos + 2);

			// Parse chunk size (hexadecimal)
			_currentChunkSize = _parseHexSize(chunkSizeLine);

			if (_currentChunkSize == 0)
			{
				_state = CHUNK_TRAILER;
			}
			else
			{
				_state = CHUNK_DATA;
				_bytesRead = 0;
			}
			break;
		}

		case CHUNK_DATA:
		{
			size_t availableBytes = _chunkBuffer.length();
			size_t neededBytes = _currentChunkSize - _bytesRead;
			size_t bytesToRead = (availableBytes < neededBytes) ? availableBytes : neededBytes;

			if (bytesToRead > 0)
			{
				_decodedData += _chunkBuffer.substr(0, bytesToRead);
				_chunkBuffer.erase(0, bytesToRead);
				_bytesRead += bytesToRead;
			}

			if (_bytesRead >= _currentChunkSize)
			{
				// Chunk complete, expect CRLF
				if (_chunkBuffer.length() >= 2 && _chunkBuffer.substr(0, 2) == "\r\n")
				{
					_chunkBuffer.erase(0, 2);
					_state = CHUNK_SIZE;
				}
				else if (_chunkBuffer.length() < 2)
				{
					// Need more data for trailing CRLF
					return _state;
				}
				else
				{
					Logger::log(Logger::ERROR, "Invalid chunk format - missing trailing CRLF");
					_state = CHUNK_ERROR;
				}
			}
			break;
		}

		case CHUNK_TRAILER:
		{
			// Look for final CRLF indicating end of trailers
			size_t crlfPos = _chunkBuffer.find("\r\n");
			if (crlfPos == std::string::npos)
			{
				return _state;
			}

			std::string trailerLine = _chunkBuffer.substr(0, crlfPos);
			_chunkBuffer.erase(0, crlfPos + 2);

			if (trailerLine.empty())
			{
				// Empty line indicates end of trailers
				_state = CHUNK_COMPLETE;
				_isComplete = true;
			}
			// Ignore trailer headers for now
			break;
		}

		case CHUNK_COMPLETE:
		case CHUNK_ERROR:
			return _state;
		}
	}

	return _state;
}

size_t ChunkedParser::_parseHexSize(const std::string &hexStr) const
{
	if (hexStr.empty())
	{
		return 0;
	}

	// Find first non-hex character (chunk extensions start with ';')
	size_t hexEnd = 0;
	while (hexEnd < hexStr.length() && _isValidHexChar(hexStr[hexEnd]))
	{
		hexEnd++;
	}

	if (hexEnd == 0)
	{
		return 0;
	}

	std::string hexPart = hexStr.substr(0, hexEnd);

	// Convert hex string to size_t
	char *endPtr;
	unsigned long size = std::strtoul(hexPart.c_str(), &endPtr, 16);

	if (*endPtr != '\0')
	{
		Logger::log(Logger::ERROR, "Invalid hex chunk size: " + hexPart);
		return 0;
	}

	return static_cast<size_t>(size);
}

/*
** --------------------------------- VALIDATION METHODS ----------------------------------
*/

bool ChunkedParser::_isValidHexChar(char c) const
{
	return (c >= '0' && c <= '9') ||
		   (c >= 'a' && c <= 'f') ||
		   (c >= 'A' && c <= 'F');
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

bool ChunkedParser::isComplete() const
{
	return _isComplete;
}

bool ChunkedParser::hasError() const
{
	return _state == CHUNK_ERROR;
}

const std::string &ChunkedParser::getDecodedData() const
{
	return _decodedData;
}

size_t ChunkedParser::getTotalSize() const
{
	return _decodedData.length();
}

void ChunkedParser::reset()
{
	_state = CHUNK_SIZE;
	_currentChunkSize = 0;
	_bytesRead = 0;
	_chunkBuffer.clear();
	_decodedData.clear();
	_isComplete = false;
}

/* ************************************************************************** */
