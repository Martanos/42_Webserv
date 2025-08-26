#include "ClientManager.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ClientManager::ClientManager()
{
}

ClientManager::ClientManager( const ClientManager & src )
{
}


/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ClientManager::~ClientManager()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ClientManager &				ClientManager::operator=( ClientManager const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, ClientManager const & i )
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