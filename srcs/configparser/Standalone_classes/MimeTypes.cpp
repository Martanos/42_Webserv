#include "MimeTypes.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

MimeTypes::MimeTypes()
{
}

MimeTypes::MimeTypes( const MimeTypes & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

MimeTypes::~MimeTypes()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

MimeTypes &				MimeTypes::operator=( MimeTypes const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, MimeTypes const & i )
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