#include "HttpRequest.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpRequest::HttpRequest()
{
	_method = "";
	_uri = "";
	_version = "";
	_headers = std::map<std::string, std::string>();
	_body = "";
	_contentLength = 0;
	_isChunked = false;
	_parseState = PARSE_REQUEST_LINE;
	_rawBuffer = "";
	_bodyBytesReceived = 0;
}

HttpRequest::HttpRequest(const HttpRequest &src)
{
	*this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

HttpRequest::~HttpRequest()
{
	_headers.clear();
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

HttpRequest &HttpRequest::operator=(HttpRequest const &rhs)
{
	if (this != &rhs)
	{
		this->_method = rhs._method;
		this->_uri = rhs._uri;
		this->_version = rhs._version;
		this->_headers = rhs._headers;
		this->_body = rhs._body;
		this->_contentLength = rhs._contentLength;
		this->_isChunked = rhs._isChunked;
		this->_parseState = rhs._parseState;
		this->_rawBuffer = rhs._rawBuffer;
		this->_bodyBytesReceived = rhs._bodyBytesReceived;
	}
	return *this;
}

std::ostream &operator<<(std::ostream &o, HttpRequest const &i)
{
	// o << "Value = " << i.getValue();
	return o;
}

/*
** --------------------------------- PARSING METHODS ----------------------------------
*/

HttpRequest::ParseState HttpRequest::parseBuffer(const std::string &buffer)
{
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

/* ************************************************************************** */
