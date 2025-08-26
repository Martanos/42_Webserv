#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <iostream>
#include <string>

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
