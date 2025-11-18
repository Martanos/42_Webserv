#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include "../../includes/Config/Server.hpp"
#include "../../includes/Http/HttpBody.hpp"
#include "../../includes/Http/HttpHeaders.hpp"
#include "../../includes/Http/HttpResponse.hpp"
#include "../../includes/Http/HttpURI.hpp"
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
// TODO: Rewrite this and related classes to an inherited class structure
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

	// Internal redirect tracking
	int _internalRedirectDepth;
	static const int MAX_INTERNAL_REDIRECTS = 5;

	// External configuration
	const std::vector<Server> *_potentialServers;
	const Server *_selectedServer;
	std::string _selectedServerHost;
	std::string _selectedServerPort;
	const Location *_selectedLocation;
	SocketAddress *_remoteAddress;
	bool _identifyServer(HttpResponse &response);
	bool _identifyLocation(HttpResponse &response);

public:
	HttpRequest();
	HttpRequest(const HttpRequest &other);
	HttpRequest &operator=(const HttpRequest &other);
	~HttpRequest();

	// Parsing methods
	ParseState parseBuffer(std::vector<char> &holdingBuffer, HttpResponse &response);

	void reset();

	// Mutators
	void setParseState(ParseState parseState);
	void setPotentialServers(const std::vector<Server> *potentialServers);
	void setSelectedServer(Server *selectedServer);
	void setSelectedLocation(const Location *selectedLocation);
	void setRemoteAddress(const SocketAddress *remoteAddress);

	// Internal redirect management
	int getInternalRedirectDepth() const;
	void incrementInternalRedirectDepth();
	void resetInternalRedirectDepth();

	// Request accessors
	ParseState getParseState() const;
	size_t getMessageSize() const;

	// Direct component access
	const HttpURI &getURI() const;
	HttpURI &getURI();
	const HttpHeaders &getHeaders() const;
	HttpHeaders &getHeaders();
	const HttpBody &getBody() const;
	HttpBody &getBody();

	// Body and size helpers
	bool isChunked();

	// Server accessors
	const std::vector<Server> *getPotentialServers() const;
	const Server *getSelectedServer() const;
	const Location *getSelectedLocation() const;
	const SocketAddress *getRemoteAddress() const;
	const std::string &getSelectedServerHost() const;
	const std::string &getSelectedServerPort() const;
};

#endif /* HTTPREQUEST_HPP */
