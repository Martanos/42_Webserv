#include "StringUtils.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

StringUtils::StringUtils()
{
}

StringUtils::StringUtils( const StringUtils & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

StringUtils::~StringUtils()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

StringUtils &				StringUtils::operator=( StringUtils const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, StringUtils const & i )
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