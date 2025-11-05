#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include "../../includes/Core/Server.hpp"
#include "../../includes/HTTP/HttpBody.hpp"
#include "../../includes/HTTP/HttpHeaders.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
#include "../../includes/HTTP/HttpURI.hpp"
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

	// External configuration
	const std::vector<Server> *_potentialServers;
	Server *_selectedServer;
	bool _identifyServer(HttpResponse &response);

public:
	HttpRequest();
	HttpRequest(const HttpRequest &other);
	HttpRequest &operator=(const HttpRequest &other);
	~HttpRequest();

	// Parsing methods
	ParseState parseBuffer(std::vector<char> &holdingBuffer, HttpResponse &response);

	// Sanitization methods
	void sanitizeRequest(HttpResponse &response, const Server *server, const Location *location);
	void reset();

	// Mutators
	void setParseState(ParseState parseState);
	void setPotentialServers(const std::vector<Server> *potentialServers);
	void setSelectedServer(Server *selectedServer);

	// Request accessors
	ParseState getParseState() const;
	size_t getMessageSize() const;

	// URI accessors
	std::string getMethod() const;
	std::string getUri() const;
	std::string getRawUri() const;
	std::string getVersion() const;

	// Headers accessors
	std::map<std::string, std::vector<std::string> > getHeaders() const;
	const std::vector<std::string> getHeader(const std::string &name) const;

	// Body accessors
	std::string getBodyData() const;
	HttpBody::BodyType getBodyType() const;
	size_t getContentLength() const;
	bool isChunked();
	bool isUsingTempFile() const;
	std::string getTempFile() const;

	// Server accessors
	const std::vector<Server> *getPotentialServers() const;
	Server *getSelectedServer() const;
};

// TODO : Add request stream overload

#endif /* HTTPREQUEST_HPP */
