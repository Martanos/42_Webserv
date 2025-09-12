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
#include "FileDescriptor.hpp"
#include "HttpURI.hpp"
#include "HttpHeaders.hpp"
#include "HttpBody.hpp"
#include "RingBuffer.hpp"

// TODO: Replace buffers with buffer wrapper
// TODO: Include check for if is a URI contains a CGI route
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
	RingBuffer _rawBuffer;
	size_t _bytesReceived;

public:
	HttpRequest();
	HttpRequest(const HttpRequest &other);
	HttpRequest &operator=(const HttpRequest &other);
	~HttpRequest();

	// Parsing methods
	ParseState parseBuffer(RingBuffer &buffer, HttpResponse &response);
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
};

#endif /* HTTPREQUEST_HPP */
