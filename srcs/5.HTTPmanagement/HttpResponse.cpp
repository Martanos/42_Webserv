#include "HttpResponse.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpResponse::HttpResponse()
{
}

HttpResponse::HttpResponse( const HttpResponse & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

HttpResponse::~HttpResponse()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

HttpResponse &				HttpResponse::operator=( HttpResponse const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, HttpResponse const & i )
{
	//o << "Value = " << i.getValue();
	return o;
}


/*
** --------------------------------- METHODS ----------------------------------
*/


/*
** --------------------------------- ACCESSOR ---------------------------------
*/


/* ************************************************************************** */