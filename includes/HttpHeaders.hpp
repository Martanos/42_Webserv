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
	struct Header
	{
		std::string value;
		}
	HeadersState _headersState;
	std::vector<std::string
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
