#include "../../includes/MethodHandlerFactory.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

MethodHandlerFactory::MethodHandlerFactory()
{
	// Initialize handlers
	_handlers["GET"] = new GetMethodHandler();
	_handlers["HEAD"] = _handlers["GET"]; // HEAD uses same handler as GET
	_handlers["POST"] = new PostMethodHandler();
	_handlers["DELETE"] = new DeleteMethodHandler();

	Logger::log(Logger::INFO, "MethodHandlerFactory initialized with GET, "
							  "HEAD, POST, DELETE handlers");
}

MethodHandlerFactory::MethodHandlerFactory(const MethodHandlerFactory &src)
{
	(void)src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

MethodHandlerFactory::~MethodHandlerFactory()
{
	delete _handlers["GET"];
	delete _handlers["POST"];
	delete _handlers["DELETE"];
	_handlers.clear();
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

MethodHandlerFactory &MethodHandlerFactory::operator=(MethodHandlerFactory const &rhs)
{
	(void)rhs;
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

MethodHandlerFactory &MethodHandlerFactory::getInstance()
{
	static MethodHandlerFactory instance;
	return instance;
}

IMethodHandler *MethodHandlerFactory::getHandler(const std::string &method) const
{
	std::map<std::string, IMethodHandler *>::const_iterator it = _handlers.find(method);
	if (it != _handlers.end())
	{
		return it->second;
	}
	return NULL;
}

bool MethodHandlerFactory::isMethodSupported(const std::string &method) const
{
	return _handlers.find(method) != _handlers.end();
}

std::vector<std::string> MethodHandlerFactory::getSupportedMethods() const
{
	std::vector<std::string> methods;
	for (std::map<std::string, IMethodHandler *>::const_iterator it = _handlers.begin(); it != _handlers.end(); ++it)
	{
		if (it->first != "HEAD")
		{ // Don't duplicate HEAD as it's handled by GET
			methods.push_back(it->first);
		}
	}
	return methods;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

/* ************************************************************************** */
