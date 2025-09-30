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
	// Constructor
	HttpURI();
	HttpURI(const HttpURI &other);
	~HttpURI();
	HttpURI &operator=(const HttpURI &other);

	// Parsing state
	enum URIState
	{
		URI_PARSING = 0,
		URI_PARSING_COMPLETE = 1,
		URI_PARSING_ERROR = 2
	};

	// Main parsing method
	void parseBuffer(std::vector<char> &buffer, HttpResponse &response);

	// Accessors
	std::string &getMethod();
	std::string &getURI();
	std::string &getVersion();
	const std::string &getMethod() const;
	const std::string &getURI() const;
	const std::string &getVersion() const;
	URIState getURIState() const;
	size_t getRawURISize() const;

	// Methods
	void reset();

private:
	URIState _uriState;
	size_t _rawURISize;

	// Request line
	std::string _method;
	std::string _uri;
	std::string _version;
};

#endif /* ********************************************************* HTTPURI_H                                          \
		*/
