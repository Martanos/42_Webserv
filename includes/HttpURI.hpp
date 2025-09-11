#ifndef HTTPURI_HPP
#define HTTPURI_HPP

#include <iostream>
#include <string>
#include "SafeBuffer.hpp"
#include "Logger.hpp"
#include "HttpResponse.hpp"
#include "Constants.hpp"
#include <sys/types.h>
#include <unistd.h>
#include <cstddef>

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
	int parseBuffer(std::string &buffer, HttpResponse &response);

	// Accessors
	URIState getURIState() const;
	std::string getRawURI() const;
	std::string getMethod() const;
	std::string getURI() const;
	std::string getVersion() const;

	// Mutators
	void setURIState(URIState uriState);
	void setRawURI(const std::string &rawURI);
	void setMethod(const std::string &method);
	void setURI(const std::string &uri);
	void setVersion(const std::string &version);

	// Methods
	void reset();

private:
	URIState _uriState;
	std::string _rawURI;

	// Request line
	std::string _method;
	std::string _uri;
	std::string _version;
};

#endif /* ********************************************************* HTTPURI_H */
