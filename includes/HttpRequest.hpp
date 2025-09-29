#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include "HttpBody.hpp"
#include "HttpHeaders.hpp"
#include "HttpURI.hpp"
#include "RingBuffer.hpp"
#include "Server.hpp"
#include <cstdlib>
#include <ctime>
#include <map>
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
		PARSING_URI = 0,
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
	std::vector<char> _rawBuffer; // Buffer for raw data
	size_t _totalBytesReceived;

public:
	HttpRequest();
	HttpRequest(const HttpRequest &other);
	HttpRequest &operator=(const HttpRequest &other);
	~HttpRequest();

	// Parsing methods
	ParseState parseBuffer(char &buffer, HttpResponse &response, Server *server);
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
