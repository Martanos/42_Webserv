#ifndef HTTPBODY_HPP
#define HTTPBODY_HPP

#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <vector>
#include "Constants.hpp"
#include "HttpResponse.hpp"
#include "FileDescriptor.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"

// The responsibility of this class is to parse the body of the request
// For example if chunked transfer encoding is used, this class will parse the body
// It also manages the storage during buffering and switching to temp file if needed
class HttpBody
{
public:
	enum BodyState
	{
		BODY_PARSING = 0,
		BODY_PARSING_COMPLETE = 1,
		BODY_PARSING_ERROR = 2
	};

	enum BodyType
	{
		BODY_TYPE_NO_BODY = 0,
		BODY_TYPE_CHUNKED = 1,
		BODY_TYPE_CONTENT_LENGTH = 2
	};

	enum ChunkState
	{
		CHUNK_SIZE = 0,
		CHUNK_DATA = 1,
		CHUNK_TRAILERS = 2,
		CHUNK_COMPLETE = 3
	};

private:
	BodyState _bodyState;
	BodyType _bodyType;
	ChunkState _chunkState;
	size_t _expectedBodySize;
	bool _isChunked;
	bool _isUsingTempFile;
	std::vector<char> _rawBodyLine;
	std::string _tempFilePath;
	FileDescriptor _tempFd;

	// File management methods
	std::string _generateTempFilePath();
	void _switchToTempFile();
	void _appendToTempFile(const std::string &data);
	void _cleanupTempFile();

	// Decoding methods
	size_t _parseHexSize(const std::string &hexStr) const;

public:
	HttpBody();
	HttpBody(HttpBody const &src);
	~HttpBody();

	HttpBody &operator=(HttpBody const &rhs);

	int parseBuffer(std::string &buffer, HttpResponse &response);

	// Accessors
	BodyState getBodyState() const;
	std::string getRawBody() const;
	size_t getBodySize() const;
	size_t getBodyBytesRead() const;
	bool getIsChunked() const;
	bool getIsUsingTempFile() const;
	std::string getTempFilePath() const;
	FileDescriptor getTempFd() const;

	// Mutators
	void setBodyState(BodyState bodyState);
	void setRawBody(const std::string &rawBody);
	void setBodySize(size_t bodySize);
	void setBodyBytesRead(size_t bodyBytesRead);
	void setIsChunked(bool isChunked);
	void setIsUsingTempFile(bool isUsingTempFile);
	void setTempFilePath(const std::string &tempFilePath);
	void setTempFd(const FileDescriptor &tempFd);

	// Methods
	void reset();
};

#endif /* ******************************************************** HTTPBODY_H */
