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
#include <limits>
#include <cstdint>
#include "Logger.hpp"
#include "HttpResponse.hpp"
#include "Constants.hpp"
#include "StringUtils.hpp"
#include "RingBuffer.hpp"
#include "HttpBody.hpp"

class HttpHeaders
{

public:
	enum HeadersState
	{
		HEADERS_PARSING = 0,
		HEADERS_PARSING_COMPLETE = 1,
		HEADERS_PARSING_ERROR = 2
	};

private:
	HeadersState _headersState;
	size_t _expectedBodySize;
	RingBuffer _rawHeaders;
	std::map<std::string, std::vector<std::string> > _headers;

public:
	// OOP
	HttpHeaders();
	HttpHeaders(HttpHeaders const &src);
	~HttpHeaders();
	HttpHeaders &operator=(HttpHeaders const &rhs);

	// Main parsing method
	int parseBuffer(RingBuffer &buffer, HttpResponse &response, HttpBody &body);
	void parseHeaderLine(const std::string &line, HttpResponse &response, HttpBody &body);
	void parseHeaders(HttpResponse &response, HttpBody &body);

	// Accessors
	int getHeadersState() const;
	RingBuffer getRawHeaders() const;
	std::map<std::string, std::vector<std::string> > getHeaders() const;
	size_t getHeadersSize() const;
	size_t getExpectedBodySize() const;

	// Mutators
	void setHeadersState(HeadersState headersState);
	void setRawHeaders(const RingBuffer &rawHeaders);
	void setHeaders(const std::map<std::string, std::vector<std::string> > &headers);
	void setExpectedBodySize(size_t expectedBodySize);

	// Methods
	void reset();
};

#endif /* ***************************************************** HTTPHEADERS_H */
