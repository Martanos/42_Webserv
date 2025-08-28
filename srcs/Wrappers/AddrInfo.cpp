#include "AddrInfo.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

AddrInfo::AddrInfo()
{
}

AddrInfo::AddrInfo( const AddrInfo & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

AddrInfo::~AddrInfo()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

AddrInfo &				AddrInfo::operator=( AddrInfo const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, AddrInfo const & i )
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