#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include "Logger.hpp"
#include "StringUtils.hpp"

// This class is used to generate the response to the client
class HttpResponse
{
private:
	int _statusCode;
	std::string _statusMessage;
	std::string _version;
	std::map<std::string, std::string> _headers;
	std::string _body;
	size_t _bytesSent;
	std::string _rawResponse;

public:
	HttpResponse();
	HttpResponse(HttpResponse const &src);
	~HttpResponse();
	HttpResponse &operator=(HttpResponse const &rhs);

	// Methods needed based on Client.cpp usage:
	void reset();
	bool isEmpty() const;
	void setStatus(int code, const std::string &message);
	void setHeader(const std::string &name, const std::string &value);
	void setBody(const std::string &body);
	std::string toString() const; // Generate full HTTP response
};

#endif /* **************************************************** HTTPRESPONSE_H */
