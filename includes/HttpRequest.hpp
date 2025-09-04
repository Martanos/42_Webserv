#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <ctime>
#include "Logger.hpp"
#include "Constants.hpp"
#include "StringUtils.hpp"
#include "SafeBuffer.hpp"
#include "FileDescriptor.hpp"

// TODO: Replace buffers with buffer wrapper
class HttpRequest
{
public:
	enum ParseState
	{
		PARSE_REQUEST_LINE = 0,
		PARSE_HEADERS = 1,
		PARSE_BODY = 2,
		PARSE_COMPLETE = 3,
		PARSE_ERROR = 4,
		PARSE_ERROR_REQUEST_LINE_TOO_LONG = 5,
		PARSE_ERROR_HEADER_TOO_LONG = 6,
		PARSE_ERROR_BODY_TOO_LONG = 7,
		PARSE_ERROR_CONTENT_LENGTH_TOO_LONG = 8,
		PARSE_ERROR_INVALID_REQUEST_LINE = 9,
		PARSE_ERROR_INVALID_HTTP_METHOD = 10,
		PARSE_ERROR_INVALID_HTTP_VERSION = 11,
		PARSE_ERROR_MALFORMED_REQUEST = 13,
		PARSE_ERROR_INTERNAL_SERVER_ERROR = 14,
		PARSE_ERROR_TEMP_FILE_ERROR = 15
	};

private:
	enum ChunkState
	{
		CHUNK_SIZE = 0,	   // Reading chunk size line
		CHUNK_DATA = 1,	   // Reading chunk data
		CHUNK_TRAILER = 2, // Reading trailer headers
		CHUNK_COMPLETE = 3 // Chunked transfer complete
	};

	// Request line
	std::string _method;
	std::string _uri;
	std::string _version;

	// Headers
	std::map<std::string, std::vector<std::string> > _headers;

	// Body handling
	std::string _body;	   // In-memory body storage
	size_t _contentLength; // Expected content length
	bool _isChunked;	   // Chunked transfer encoding flag

	// Chunked parsing state
	ChunkState _chunkState;		   // Current chunk parsing state
	size_t _currentChunkSize;	   // Size of current chunk being read
	size_t _currentChunkBytesRead; // Bytes read of current chunk
	size_t _totalBodySize;		   // Total body size accumulated

	// Temp file handling for large bodies
	bool _usingTempFile;	   // Flag if body is stored in temp file
	std::string _tempFilePath; // Path to temp file
	FileDescriptor _tempFd;	   // File descriptor for temp file

	// Message limits
	size_t _maxRequestLineSize;
	size_t _maxHeaderSize;
	size_t _maxBodySize;
	size_t _maxContentLength;

	// Parsing state
	ParseState _parseState;
	std::string _rawBuffer;
	size_t _bytesReceived;

	// temp FD for flushing
	bool _isUsingTempFile;
	std::string _tempFilePath;
	FileDescriptor _tempFd;

public:
	HttpRequest();
	HttpRequest(const HttpRequest &other);
	HttpRequest &operator=(const HttpRequest &other);
	~HttpRequest();

	// Parsing methods
	ParseState parseBuffer(const std::string &buffer, ssize_t bodyBufferSize);
	bool isComplete() const;
	bool hasError() const;
	void reset();

	// Getters
	const std::string &getMethod() const;
	const std::string &getUri() const;
	const std::string &getVersion() const;
	const std::map<std::string, std::vector<std::string> > &getHeaders() const;
	const std::vector<std::string> &getHeader(const std::string &name) const;
	const std::string &getBody() const;
	size_t getContentLength() const;
	bool isChunked() const;
	bool isUsingTempFile() const;
	std::string getTempFile() const;
	std::string _getBodyFromFile() const;

private:
	ParseState _parseRequestLine(const std::string &line);
	ParseState _parseHeaderLine(const std::string &line);
	ParseState _parseBody();
	ParseState _parseChunkedBody();
	ParseState _parseHeaders();
	ParseState _parseChunkSize();
	ParseState _parseChunkData();
	ParseState _parseChunkTrailers();
	ParseState _switchToTempFile();
	ParseState _appendToTempFile(const std::string &data);
	size_t _parseHexSize(const std::string &hexStr) const;
	ParseState _handleBodyData(const std::string &data);
	void _cleanupTempFile();
	std::string _createTempFilePath() const;
	bool _isValidMethod(const std::string &method) const;
	std::string _toLowerCase(const std::string &str) const;
};

#endif /* HTTPREQUEST_HPP */
