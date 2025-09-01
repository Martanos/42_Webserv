#include "DeleteMethodHandler.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

DeleteMethodHandler::DeleteMethodHandler()
{
}

DeleteMethodHandler::DeleteMethodHandler( const DeleteMethodHandler & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

DeleteMethodHandler::~DeleteMethodHandler()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

DeleteMethodHandler &				DeleteMethodHandler::operator=( DeleteMethodHandler const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, DeleteMethodHandler const & i )
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