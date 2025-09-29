#ifndef HTTPHEADERS_HPP
#define HTTPHEADERS_HPP

#include "Constants.hpp"
#include "HttpBody.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <ctime>
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
	std::map<std::string, std::vector<std::string> > _headers;
	size_t _expectedBodySize;

public:
	// OOP
	HttpHeaders();
	HttpHeaders(HttpHeaders const &src);
	~HttpHeaders();
	HttpHeaders &operator=(HttpHeaders const &rhs);

	// Main parsing method
	void parseBuffer(std::vector<char> &buffer, HttpResponse &response, HttpBody &body);
	void parseHeaderLine(const std::string &line, HttpResponse &response, HttpBody &body);
	void parseHeaders(const std::string &headersData, HttpResponse &response, HttpBody &body);

	// Accessors
	int getHeadersState() const;
	std::map<std::string, std::vector<std::string> > getHeaders() const;
	size_t getHeadersSize() const;
	size_t getExpectedBodySize() const;

	// Methods
	void reset();
};

#endif /* ***************************************************** HTTPHEADERS_H                                          \
		*/
