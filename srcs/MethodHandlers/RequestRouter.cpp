#include "RequestRouter.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

RequestRouter::RequestRouter()
{
}

RequestRouter::RequestRouter( const RequestRouter & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

RequestRouter::~RequestRouter()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

RequestRouter &				RequestRouter::operator=( RequestRouter const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, RequestRouter const & i )
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