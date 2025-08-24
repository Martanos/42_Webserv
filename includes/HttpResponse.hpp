#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <iostream>

class HttpResponse
{
private:
	int _statusCode;
	std::string _statusMessage;
	std::map<std::string, std::string> _headers;
	std::string _body;

	// Sending state
	std::string _rawResponse;
	size_t _bytesSent;
	bool _headersSent;

public:
	HttpResponse();
	HttpResponse(const HttpResponse &other);
	HttpResponse &operator=(const HttpResponse &other);
	~HttpResponse();

	// Status
	void setStatus(int code, const std::string &message = "");
	int getStatusCode() const { return _statusCode; }
	const std::string &getStatusMessage() const { return _statusMessage; }

	// Headers
	void setHeader(const std::string &name, const std::string &value);
	void removeHeader(const std::string &name);
	const std::string &getHeader(const std::string &name) const;

	// Body
	void setBody(const std::string &body);
	void appendBody(const std::string &data);
	const std::string &getBody() const { return _body; }
	void clearBody();

	// Response generation
	std::string generateResponse();
	void reset();

	// Sending state
	size_t getBytesSent() const { return _bytesSent; }
	void addBytesSent(size_t bytes) { _bytesSent += bytes; }
	bool isComplete() const;

private:
	void _setDefaultHeaders();
	std::string _formatHeaders() const;
};

#endif /* ***************************************************** HTTPREPONSE_H */
