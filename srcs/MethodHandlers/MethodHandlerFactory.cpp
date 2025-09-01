#include "MethodHandlerFactory.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

MethodHandlerFactory::MethodHandlerFactory()
{
}

MethodHandlerFactory::MethodHandlerFactory( const MethodHandlerFactory & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

MethodHandlerFactory::~MethodHandlerFactory()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

MethodHandlerFactory &				MethodHandlerFactory::operator=( MethodHandlerFactory const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, MethodHandlerFactory const & i )
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