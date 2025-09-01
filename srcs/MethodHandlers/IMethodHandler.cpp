#include "IMethodHandler.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

IMethodHandler::IMethodHandler()
{
}

IMethodHandler::IMethodHandler( const IMethodHandler & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

IMethodHandler::~IMethodHandler()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

IMethodHandler &				IMethodHandler::operator=( IMethodHandler const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, IMethodHandler const & i )
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