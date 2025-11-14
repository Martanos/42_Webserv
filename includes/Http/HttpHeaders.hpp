#ifndef HTTPHEADERS_HPP
#define HTTPHEADERS_HPP

#include "../../includes/Http/Header.hpp"
#include "../../includes/Http/HttpBody.hpp"
#include "../../includes/Http/HttpResponse.hpp"
#include <cstddef>
#include <cstdlib>
#include <ctime>
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
	std::vector<Header> _headers;
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
	const std::vector<Header> &getHeaders() const;
	const Header *getHeader(const std::string &headerName) const;
	size_t getHeadersSize() const;

	// Methods
	bool isSingletonHeader(const std::string &headerName) const;
	void reset();
};

#endif /* ***************************************************** HTTPHEADERS_H                                          \
		*/
