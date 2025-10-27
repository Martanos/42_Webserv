<<<<<<< HEAD:includes/HttpHeaders.hpp
#ifndef HTTPHEADERS_HPP
#define HTTPHEADERS_HPP

#include "Constants.hpp"
#include "HttpBody.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "RingBuffer.hpp"
#include "StringUtils.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

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

#endif /* ***************************************************** HTTPHEADERS_H                                          \
		*/
=======
#ifndef HTTPHEADERS_HPP
#define HTTPHEADERS_HPP

#include "../../includes/HTTP/HttpBody.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <map>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

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
	struct Header
	{
		std::string value;
	};

	HeadersState _headersState;
	std::map<std::string, std::vector<std::string> > _headers;
	size_t _rawHeadersSize;

	// Helper methods
	void parseHeaderLine(const std::string &line, HttpResponse &response);
	void parseAllHeaders(HttpResponse &response, HttpBody &body);

public:
	// OOP
	HttpHeaders();
	HttpHeaders(HttpHeaders const &src);
	~HttpHeaders();
	HttpHeaders &operator=(HttpHeaders const &rhs);

	// Main parsing method
	void parseBuffer(std::vector<char> &buffer, HttpResponse &response, HttpBody &body);

	// Accessors
	int getHeadersState() const;
	const std::map<std::string, std::vector<std::string> > &getHeaders() const;
	const std::vector<std::string> getHeader(const std::string &headerName) const;
	size_t getHeadersSize() const;

	// Methods
	bool isSingletonHeader(const std::string &headerName);
	void reset();
};

#endif /* ***************************************************** HTTPHEADERS_H                                          \
		*/
>>>>>>> ConfigParserRefactor:includes/HTTP/HttpHeaders.hpp
