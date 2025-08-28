#include "ListeningSocket.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ListeningSocket::ListeningSocket()
{
}

ListeningSocket::ListeningSocket( const ListeningSocket & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ListeningSocket::~ListeningSocket()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ListeningSocket &				ListeningSocket::operator=( ListeningSocket const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, ListeningSocket const & i )
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