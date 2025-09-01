#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include "Logger.hpp"
#include "StringUtils.hpp"

// TODO:IMPLEMENT ACCESSORS AND MUTATORS
//  This class is used to generate the response to the client
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

	// Accessors
	int getStatusCode() const;
	std::string getStatusMessage() const;
	std::string getVersion() const;
	std::map<std::string, std::string> getHeaders() const;
	std::string getBody() const;
	size_t getBytesSent() const;
	std::string getRawResponse() const;
	bool isEmpty() const;

	// Mutators
	void setHeader(const std::string &name, const std::string &value);
	void setStatusCode(int code);
	void setStatusMessage(const std::string &message);
	void setStatus(int code, const std::string &message);
	void setVersion(const std::string &version);
	void setHeaders(const std::map<std::string, std::string> &headers);
	void setBody(const std::string &body);
	void setBytesSent(size_t bytesSent);
	void setRawResponse(const std::string &rawResponse);

	// Methods
	std::string toString() const;
};

#endif /* **************************************************** HTTPRESPONSE_H */
