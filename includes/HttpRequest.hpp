#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include "Constants.hpp"
#include "FileDescriptor.hpp"
#include "HttpBody.hpp"
#include "HttpHeaders.hpp"
#include "HttpURI.hpp"
#include "Logger.hpp"
#include "RingBuffer.hpp"
#include "StringUtils.hpp"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <map>
#include <sstream>
#include <string>
#include <vector>

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
	std::string getMethod() const;
	std::string getUri() const;
	std::string getVersion() const;
	std::map<std::string, std::vector<std::string> > getHeaders() const;
	const std::vector<std::string> &getHeader(const std::string &name) const;
	std::string getBody() const;
	size_t getContentLength() const;
	bool isChunked() const;
	bool isUsingTempFile() const;
	std::string getTempFile() const;
};

#endif /* HTTPREQUEST_HPP */
