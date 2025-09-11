#include "HttpBody.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpBody::HttpBody()
{
}

HttpBody::HttpBody( const HttpBody & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

HttpBody::~HttpBody()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

HttpBody &				HttpBody::operator=( HttpBody const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, HttpBody const & i )
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