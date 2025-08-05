#include "ResponseMap.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ResponseMap::ResponseMap()
{
}

ResponseMap::ResponseMap( const ResponseMap & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ResponseMap::~ResponseMap()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ResponseMap &				ResponseMap::operator=( ResponseMap const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, ResponseMap const & i )
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