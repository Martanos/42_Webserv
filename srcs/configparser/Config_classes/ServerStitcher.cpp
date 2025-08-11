#include "ServerStitcher.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ServerStitcher::ServerStitcher()
{
}

ServerStitcher::ServerStitcher( const ServerStitcher & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ServerStitcher::~ServerStitcher()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ServerStitcher &				ServerStitcher::operator=( ServerStitcher const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, ServerStitcher const & i )
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