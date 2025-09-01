#include "DeleteMethodHandler.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

DeleteMethodHandler::DeleteMethodHandler()
{
}

DeleteMethodHandler::DeleteMethodHandler(const DeleteMethodHandler &src)
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

DeleteMethodHandler &DeleteMethodHandler::operator=(DeleteMethodHandler const &rhs)
{
	// if ( this != &rhs )
	//{
	// this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &operator<<(std::ostream &o, DeleteMethodHandler const &i)
{
	// o << "Value = " << i.getValue();
	return o;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void DeleteMethodHandler::handle(const HttpRequest &request,
								 HttpResponse &response,
								 const Server *server,
								 const Location *location)
{

	throw std::runtime_error("DeleteMethodHandler::handle not implemented");
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

bool DeleteMethodHandler::canHandle(const std::string &method) const
{
	return method == "DELETE";
}

std::string DeleteMethodHandler::getMethod() const
{
	return "DELETE";
}

bool DeleteMethodHandler::canDeleteFile(const std::string &filePath,
										const Location *location) const
{
	return true;
}

/* ************************************************************************** */
