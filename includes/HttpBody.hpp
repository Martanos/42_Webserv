#ifndef HTTPBODY_HPP
#define HTTPBODY_HPP

#include <iostream>
#include <string>
#include "HttpResponse.hpp"

class HttpBody
{
public:
	enum BodyState
	{
		BODY_PARSING = 0,
		BODY_PARSING_COMPLETE = 1,
		BODY_PARSING_ERROR = 2
	};

public:
	HttpBody();
	HttpBody(HttpBody const &src);
	~HttpBody();

	HttpBody &operator=(HttpBody const &rhs);

	int parseBuffer(std::string &buffer, HttpResponse &response);

	// Accessors
	BodyState getBodyState() const;

	// Mutators
	void setBodyState(BodyState bodyState);

	// Methods
	void reset();

private:
};

#endif /* ******************************************************** HTTPBODY_H */
