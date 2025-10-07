#ifndef HTTPURI_HPP
#define HTTPURI_HPP

#include "Constants.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include <cstddef>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// This class is responsible for parsing the URI from the request line
// It will also format the reponse if anything goes wrong
class HttpURI
{
public:
	// Parsing state
	enum URIState
	{
		URI_PARSING = 0,
		URI_PARSING_COMPLETE = 1,
		URI_PARSING_ERROR = 2
	};

private:
	URIState _uriState;
	size_t _uriSize;

	// Request line
	std::string _method;
	std::string _URI;
	std::string _version;

	// Query parameters
	std::map<std::string, std::vector<std::string> > _queryParameters;

public:
	// Constructor
	HttpURI();
	HttpURI(const HttpURI &other);
	~HttpURI();
	HttpURI &operator=(const HttpURI &other);

	// Main parsing method
	void parseBuffer(std::vector<char> &buffer, HttpResponse &response);
	void sanitizeURI(const Server *server, const Location *location);

	// Accessors
	const std::string &getURI() const;
	const std::string &getVersion() const;
	const std::string &getMethod() const;
	const std::map<std::string, std::vector<std::string> > &getQueryParameters() const;
	URIState getURIState() const;

	// Methods
	void reset();
};

#endif /* ********************************************************* HTTPURI_H                                          \
		*/
