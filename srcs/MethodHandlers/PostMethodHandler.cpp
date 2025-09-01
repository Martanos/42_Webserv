#include "PostMethodHandler.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

PostMethodHandler::PostMethodHandler()
{
}

PostMethodHandler::PostMethodHandler( const PostMethodHandler & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

PostMethodHandler::~PostMethodHandler()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

PostMethodHandler &				PostMethodHandler::operator=( PostMethodHandler const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, PostMethodHandler const & i )
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