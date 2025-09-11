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
#include "HttpURI.hpp"
#include "HttpHeaders.hpp"
#include "HttpBody.hpp"

// TODO: Replace buffers with buffer wrapper
// This class ingests and parses http requests recieved from the client
// It provides an interface for accessing the request data
// HttpRequest handles http operations and response formatting
// HttpURI handles the uri parsing and validation
// HttpHeaders parses and stores the headers
// HttpBody handles the body it has streaming capability as well
class HttpRequest
{
public:
	enum ParseState
	{
		PARSING_REQUEST_LINE = 0,
		PARSING_HEADERS = 1,
		PARSING_BODY = 2,
		PARSING_COMPLETE = 3,
		PARSING_ERROR = 4
	};

private:
	// Parsed message objects
	HttpURI _uri;
	HttpHeaders _headers;
	HttpBody _body;

	// Parsing state
	ParseState _parseState;
	std::string _rawBuffer;
	size_t _bytesReceived;

	// Potential servers

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
	ParseState parseBuffer(const std::string &buffer, HttpResponse &response);
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
