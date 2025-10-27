<<<<<<< HEAD:includes/HttpBody.hpp
#ifndef HTTPBODY_HPP
#define HTTPBODY_HPP

#include "FileDescriptor.hpp"
#include "FileManager.hpp"
#include "HttpResponse.hpp"
#include "RingBuffer.hpp"
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// The responsibility of this class is to parse the body of the request
// For example if chunked transfer encoding is used, this class will parse the
// body It also manages the storage during buffering and switching to temp file
// if needed
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
	RingBuffer _chunkedBuffer;
	RingBuffer _rawBody;
	FileManager _tempChunkedFile;
	FileManager _tempFile;
	bool _isUsingTempFile;

	// Parsing paths
	BodyState _parseChunkedBody(RingBuffer &buffer, HttpResponse &response);
	BodyState _parseContentLengthBody(RingBuffer &buffer, HttpResponse &response);

	// Decoding methods
	size_t _parseHexSize(const std::string &hexStr) const;

public:
	HttpBody();
	HttpBody(HttpBody const &src);
	~HttpBody();

	HttpBody &operator=(HttpBody const &rhs);

	int parseBuffer(RingBuffer &buffer, HttpResponse &response);

	// Accessors
	BodyState getBodyState() const;
	std::string getRawBody() const;
	size_t getBodySize() const;
	size_t getBodyBytesRead() const;
	bool getIsChunked() const;
	bool getIsUsingTempFile() const;
	std::string getTempFilePath() const;
	FileDescriptor &getTempFd();

	// Mutators
	void setBodyState(BodyState bodyState);
	void setBodyType(BodyType bodyType);
	void setExpectedBodySize(size_t expectedBodySize);
	void setBodySize(size_t bodySize);
	void setIsUsingTempFile(bool isUsingTempFile);
	void setTempFilePath(const std::string &tempFilePath);
	void setTempFd(const FileDescriptor &tempFd);

	// Methods
	void reset();
};

#endif /* ******************************************************** HTTPBODY_H                                          \
		*/
=======
#ifndef HTTPBODY_HPP
#define HTTPBODY_HPP

#include "../../includes/HTTP/HttpResponse.hpp"
#include "../../includes/Wrapper/FileDescriptor.hpp"
#include "../../includes/Wrapper/FileManager.hpp"
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// The responsibility of this class is to parse the body of the request
// For example if chunked transfer encoding is used, this class will parse the
// body It also manages the storage during buffering and switching to temp file
// if needed
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
		CHUNK_COMPLETE = 3,
		CHUNK_ERROR = 4
	};

private:
	BodyState _bodyState;
	BodyType _bodyType;
	ChunkState _chunkState;
	ssize_t _expectedBodySize;
	std::vector<char> _rawBody;
	ssize_t _rawBodySize;
	FileManager _tempFile;
	bool _isUsingTempFile;

	// Parsing paths
	BodyState _parseChunkedBody(std::vector<char> &buffer, HttpResponse &response);
	BodyState _parseContentLengthBody(std::vector<char> &buffer, HttpResponse &response);

	// Decoding methods
	ssize_t _parseHexSize(const std::string &hexStr) const;

public:
	HttpBody();
	HttpBody(HttpBody const &src);
	~HttpBody();

	HttpBody &operator=(HttpBody const &rhs);

	void parseBuffer(std::vector<char> &buffer, HttpResponse &response);

	// Accessors
	BodyState getBodyState() const;
	BodyType getBodyType() const;
	std::string getRawBody() const;
	size_t getBodySize() const;
	size_t getRawBodySize() const;
	bool getIsUsingTempFile();
	std::string getTempFilePath();
	FileDescriptor &getTempFd();

	// Mutators
	void setBodyState(BodyState bodyState);
	void setBodyType(BodyType bodyType);
	void setExpectedBodySize(size_t expectedBodySize);
	void setIsUsingTempFile(bool isUsingTempFile);
	void setTempFilePath(const std::string &tempFilePath);
	void setTempFd(const FileDescriptor &tempFd);

	// Methods
	void reset();
};

#endif /* ******************************************************** HTTPBODY_H                                          \
		*/
>>>>>>> ConfigParserRefactor:includes/HTTP/HttpBody.hpp
