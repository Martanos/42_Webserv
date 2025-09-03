// HttpRequest.hpp
#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <sys/types.h>
#include <unistd.h>
#include "Logger.hpp"

// This class is used to receive the raw data and parse it into a request object
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
		PARSE_ERROR_INTERNAL = 5,
		PARSE_ERROR_INVALID_METHOD = 6,
		PARSE_ERROR_INVALID_REQUEST_LINE = 7,
		PARSE_PAYLOAD_TOO_LARGE_BODY = 8,
		PARSE_PAYLOAD_TOO_LARGE_HEADERS = 9,
		PARSE_PAYLOAD_TOO_LARGE_REQUEST_LINE = 10
	};

	enum BodyState
	{
		BODY_STATE_CHUNKED = 0,
		BODY_STATE_CONTENT_LENGTH = 1,
		BODY_STATE_COMPLETE = 2,
		BODY_STATE_ERROR = 3
	};

private:
	// Request line
	std::string _method;
	std::string _uri;
	std::string _version;

	// Headers
	std::map<std::string, std::string> _headers;

	// Body
	std::string _body;
	size_t _contentLength;
	bool _isChunked;

	// Parsing state
	ParseState _parseState;
	std::string _rawBuffer;
	size_t _bodyBytesReceived;

public:
	HttpRequest();
	HttpRequest(const HttpRequest &other);
	HttpRequest &operator=(const HttpRequest &other);
	~HttpRequest();

	// Parsing methods
	ParseState parseBuffer(const std::string &buffer, size_t bodyBufferSize);
	bool isComplete() const;
	bool hasError() const;
	void reset();

	// Getters
	const std::string &getMethod() const;
	const std::string &getUri() const;
	const std::string &getVersion() const;
	const std::map<std::string, std::string> &getHeaders() const;
	const std::string &getHeader(const std::string &name) const;
	const std::string &getBody() const;
	const std::string &getServer() const;
	size_t getContentLength() const;
	bool isChunked() const;

private:
	ParseState _parseRequestLine(const std::string &line);
	ParseState _parseHeaderLine(const std::string &line);
	ParseState _parseBody();
	void _prepareForBody();
	bool _isValidMethod(const std::string &method) const;
	std::string _toLowerCase(const std::string &str) const;
};

#endif
