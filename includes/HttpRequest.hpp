#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <algorithm>
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
		PARSE_ERROR_REQUEST_LINE_TOO_LONG = 6,
		PARSE_ERROR_HEADER_TOO_LONG = 7,
		PARSE_ERROR_BODY_TOO_LONG = 8,
		PARSE_ERROR_CONTENT_LENGTH_TOO_LONG = 9,
		PARSE_ERROR_INVALID_REQUEST_LINE = 9,
		PARSE_ERROR_INVALID_HTTP_METHOD = 11,
		PARSE_ERROR_INVALID_HTTP_VERSION = 12,
		PARSE_ERROR_MALFORMED_REQUEST = 13,
		PARSE_ERROR_INTERNAL_SERVER_ERROR = 14
	};

private:
	// Request line
	std::string _method;
	std::string _uri;
	std::string _version;

	// Headers
	std::map<std::string, std::vector<std::string> > _headers;

	// Body handling
	std::string _body;
	double _contentLength;
	bool _isChunked;

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

private:
	ParseState _parseRequestLine(const std::string &line);
	ParseState _parseHeaderLine(const std::string &line);
	ParseState _parseBody();
	ParseState _parseChunkedBody();
	ParseState _parseHeaders();
	bool _isValidMethod(const std::string &method) const;
	std::string _toLowerCase(const std::string &str) const;
};

#endif /* HTTPREQUEST_HPP */
