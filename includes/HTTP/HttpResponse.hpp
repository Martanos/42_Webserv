#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "../../includes/Core/Location.hpp"
#include "../../includes/Core/Server.hpp"
#include "../../includes/HTTP/Header.hpp"
#include "../../includes/Wrapper/FileDescriptor.hpp"
#include <string>

namespace HTTP_RESPONSE_DEFAULT
{
const std::string DEFAULT_STATUS_MESSAGE = "OK";
const int DEFAULT_STATUS_CODE = 200;
const std::string VERSION = "HTTP/1.1";
const std::string SERVER = "42_Webserv/1.0";
const std::string CONTENT_TYPE = "text/html";
} // namespace HTTP_RESPONSE_DEFAULT

// Http Response class facilitates the creation and management of HTTP responses
// It provides methods for setting the status code, status message, headers, body, and sending the response
// It also provides methods for getting the status code, status message, headers, body, and sending the response
class HttpResponse
{
public: // State of the response
	enum SendingState
	{
		RESPONSE_FORMATTING_MESSAGE = 0,
		RESPONSE_SENDING_MESSAGE = 1,
		RESPONSE_SENDING_BODY = 2,
		RESPONSE_SENDING_COMPLETE = 3,
		RESPONSE_SENDING_ERROR = 4
	};

	enum ResponseType
	{
		SUCCESS = 0,
		ERROR = 1,
		FATAL_ERROR = 2
	};

private:
	// State of the response
	SendingState _sendingState;
	ResponseType _responseType;

	// Response data
	std::string _rawResponse;

	// URI Portion
	int _statusCode;
	std::string _statusMessage;
	std::string _version;

	// Headers Portion
	std::vector<Header> _headers;

	// Body Portion
	std::string _body;
	bool _streamBody;
	FileDescriptor _bodyFileDescriptor;

	// Private methods
	void _getDateHeader();
	void _setServerHeader();
	void _setContentLengthHeader();
	void _setVersionHeader();

public:
	HttpResponse();
	HttpResponse(HttpResponse const &src);
	~HttpResponse();
	HttpResponse &operator=(HttpResponse const &rhs);

	// Accessors
	int getStatusCode() const;
	std::string getStatusMessage() const;
	std::string getVersion() const;
	std::vector<Header> getHeaders() const;
	std::string getBody() const;
	std::string getRawResponse() const;
	SendingState getSendingState() const;
	ResponseType getResponseType() const;

	// Mutators
	void setHeader(const Header &header);
	void insertHeader(const Header &header);
	void setStatusCode(int code);
	void setStatusMessage(const std::string &message);
	void setStatus(int code, const std::string &message);
	void setVersion(const std::string &version);
	void setHeaders(const std::vector<Header> &headers);
	void setBody(const std::string &body);
	void setBody(const Location *location, const Server *server);
	void setRawResponse(const std::string &rawResponse);
	void setLastModifiedHeader();

	// Methods
	void setResponseDefaultBody(int statusCode, const std::string &statusMessage, const Server *server,
								const Location *location, ResponseType responseType);
	void setResponseCustomBody(int statusCode, const std::string &statusMessage, const std::string &body,
							   const std::string &contentType, ResponseType responseType);
	void setResponseFile(int statusCode, const std::string &statusMessage, const std::string &filePath,
						 const std::string &contentType, ResponseType responseType);
	void setRedirectResponse(const std::string &redirectPath, ResponseType responseType);
	std::string toString() const;
	void sendResponse(const FileDescriptor &clientSocketFd, ssize_t &totalBytesSent);
	void reset();
};

#endif /* **************************************************** HTTPRESPONSE_H                                          \
		*/
