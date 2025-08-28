#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <iostream>
#include <string>

// TODO: Implement this
// This class is used to generate the response to the client
class HttpResponse
{

public:
	HttpResponse();
	HttpResponse(HttpResponse const &src);
	~HttpResponse();

	HttpResponse &operator=(HttpResponse const &rhs);

private:
};

std::ostream &operator<<(std::ostream &o, HttpResponse const &i);

#endif /* **************************************************** HTTPRESPONSE_H */
