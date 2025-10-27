#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include <iostream>
#include <map>
#include <sstream>
#include <string>

// TODO: Add auto formatting and auto sending for this
class HttpResponse
{
private:
	enum HttpResponseState
	{
		RESPONSE_INITIAL = 0,
		RESPONSE_SENDING = 1,
		RESPONSE_ERROR = 2
	};

	enum HttpResponseType
	{
		NORMAL_RESPONSE = 0,
		ERROR_RESPONSE = 1,
		PROTOCOL_LEVEL_ERROR = 0,	// If this occurs send a basic error response then close the connection
		APPLICATION_LEVEL_ERROR = 1 // If this occurs send a basic error response keep the connection open depending on
	};

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
	void reset();
	void setAutoFields(); // Automatically set fields based on status code
};

#endif /* **************************************************** HTTPRESPONSE_H                                          \
		*/
