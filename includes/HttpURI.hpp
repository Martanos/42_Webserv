#ifndef HTTPURI_HPP
#define HTTPURI_HPP

#include "Constants.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "RingBuffer.hpp"
#include <cstddef>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <unistd.h>

// This class is responsible for parsing the URI from the request line
// It will also format the reponse if anything goes wrong
class HttpURI
{
public:
	enum URIState
	{
		URI_PARSING = 0,
		URI_PARSING_COMPLETE = 1,
		URI_PARSING_ERROR = 2
	};

	// Main parsing method
	int parseBuffer(RingBuffer &buffer, HttpResponse &response);

	// Accessors
	URIState getURIState() const;
	RingBuffer &getRawURI();
	std::string getMethod() const;
	std::string getURI() const;
	std::string getVersion() const;

	// Mutators
	void setURIState(URIState uriState);
	void setRawURI(RingBuffer &rawURI);
	void setMethod(const std::string &method);
	void setURI(const std::string &uri);
	void setVersion(const std::string &version);

	// Methods
	void reset();

private:
	URIState _uriState;
	RingBuffer _rawURI;

	// Request line
	std::string _method;
	std::string _uri;
	std::string _version;
};

#endif /* ********************************************************* HTTPURI_H                                          \
		*/
