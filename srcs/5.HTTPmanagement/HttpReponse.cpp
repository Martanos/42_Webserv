#include "HttpReponse.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpReponse::HttpReponse()
{
}

HttpReponse::HttpReponse( const HttpReponse & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

HttpReponse::~HttpReponse()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

HttpReponse &				HttpReponse::operator=( HttpReponse const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, HttpReponse const & i )
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