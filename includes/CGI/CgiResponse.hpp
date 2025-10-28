#ifndef CGIRESPONSE_HPP
#define CGIRESPONSE_HPP

#include "../HTTP/HttpResponse.hpp"
#include "../Global/Logger.hpp"
#include <map>
#include <string>
#include <vector>

// TODO: Merge this with HttpResponse
class CgiResponse
{
public:
	enum ParseResult
	{
		SUCCESS = 0,
		ERROR_INVALID_HEADERS = 1,
		ERROR_MISSING_CONTENT_TYPE = 2,
		ERROR_INVALID_STATUS = 3,
		ERROR_MALFORMED_RESPONSE = 4
	};

private:
	std::map<std::string, std::string> _headers;
	std::string _body;
	int _statusCode;
	std::string _statusMessage;
	bool _isParsed;
	bool _isNPH; // Non-Parsed Header mode

public:
	CgiResponse();
	CgiResponse(const CgiResponse &other);
	~CgiResponse();

	CgiResponse &operator=(const CgiResponse &other);

	// Main parsing method
	ParseResult parseOutput(const std::string &cgiOutput);

	// Convert to HttpResponse
	void populateHttpResponse(HttpResponse &httpResponse) const;

	// Getters
	const std::map<std::string, std::string> &getHeaders() const;
	const std::string &getBody() const;
	int getStatusCode() const;
	const std::string &getStatusMessage() const;
	bool isParsed() const;
	bool isNPH() const;

	// Header access
	std::string getHeader(const std::string &name) const;
	bool hasHeader(const std::string &name) const;

	// Utility methods
	void clear();

private:
	// Internal parsing methods
	ParseResult parseHeaders(const std::string &headerSection);
	ParseResult parseStatusLine(const std::string &statusLine);
	void parseHeaderLine(const std::string &line);

	// Utility methods
	std::string toLowerCase(const std::string &str) const;
	std::string trim(const std::string &str) const;
	bool isValidStatusCode(int code) const;
	std::string getDefaultStatusMessage(int code) const;

	// Header processing
	void processSpecialHeaders();
	void setDefaultHeaders();
};

#endif /* CGIRESPONSE_HPP */
