#include "RequestRouting.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

RequestRouting::RequestRouting()
{
}

RequestRouting::RequestRouting( const RequestRouting & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

RequestRouting::~RequestRouting()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

RequestRouting &				RequestRouting::operator=( RequestRouting const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, RequestRouting const & i )
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