#ifndef HTTPHEADERS_HPP
#define HTTPHEADERS_HPP

#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <ctime>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <cstddef>
#include "Logger.hpp"
#include "HttpResponse.hpp"
#include "Constants.hpp"
#include "StringUtils.hpp"

class HttpHeaders
{

public:
	enum HeadersState
	{
		HEADERS_PARSING = 0,
		HEADERS_PARSING_COMPLETE = 1,
		HEADERS_PARSING_ERROR = 2
	};

	enum BodyType
	{
		BODY_TYPE_NO_BODY = 0,
		BODY_TYPE_CHUNKED = 1,
		BODY_TYPE_CONTENT_LENGTH = 2,
		BODY_TYPE_MULTIPART = 3,
	};

private:
	HeadersState _headersState;
	BodyType _bodyType;
	size_t _expectedBodySize;
	std::string _rawHeaders;
	std::map<std::string, std::vector<std::string> > _headers;

public:
	// OOP
	HttpHeaders();
	HttpHeaders(HttpHeaders const &src);
	~HttpHeaders();
	HttpHeaders &operator=(HttpHeaders const &rhs);

	// Main parsing method
	int parseBuffer(std::string &buffer, HttpResponse &response);
	void parseHeaderLine(const std::string &line, HttpResponse &response);
	void parseHeaders(HttpResponse &response);

	// Accessors
	int getHeadersState() const;
	std::string getRawHeaders() const;
	std::map<std::string, std::vector<std::string> > getHeaders() const;
	size_t getHeadersSize() const;
	BodyType getBodyType() const;
	size_t getExpectedBodySize() const;

	// Mutators
	void setHeadersState(HeadersState headersState);
	void setRawHeaders(const std::string &rawHeaders);
	void setHeaders(const std::map<std::string, std::vector<std::string> > &headers);
	void setBodyType(BodyType bodyType);
	void setExpectedBodySize(size_t expectedBodySize);

	// Methods
	void reset();
};

#endif /* ***************************************************** HTTPHEADERS_H */
