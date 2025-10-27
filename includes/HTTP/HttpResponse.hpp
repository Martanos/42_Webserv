#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "../../includes/Core/Location.hpp"
#include "../../includes/Core/Server.hpp"
#include "../../includes/HTTP/Header.hpp"
#include <map>
#include <string>

// General response class
class HttpResponse
{
private:
	// State of the response
	enum HttpResponseState
	{
		RESPONSE_SENDING = 0,
		RESPONSE_SENT = 1,
		RESPONSE_ERROR = 2
	};

	// Response data
	int _statusCode;
	std::string _statusMessage;
	std::string _version;
	std::vector<Header> _headers;
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
	void setHeaders(const std::vector<Header> &headers);
	void setBody(const std::string &body);
	void setBody(const Location *location, const Server *server);
	void setBytesSent(size_t bytesSent);
	void setRawResponse(const std::string &rawResponse);

	// Methods
	std::string toString() const;
	void reset();
	void setAutoFields(); // Automatically set fields based on status code
};

#endif /* **************************************************** HTTPRESPONSE_H                                          \
		*/
