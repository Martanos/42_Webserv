#include "../../includes/MethodHandlers/MethodHandlerFactory.hpp"
#include <vector>

// Static member definitions
std::map<std::string, MethodHandlerFactory::HandlerCreator> MethodHandlerFactory::_handlerCreators;
bool MethodHandlerFactory::_initialized = false;

MethodHandlerFactory::MethodHandlerFactory()
{
	// Private constructor
}

MethodHandlerFactory::MethodHandlerFactory(const MethodHandlerFactory &other)
{
	(void)other;
	// Private constructor
}

MethodHandlerFactory &MethodHandlerFactory::operator=(const MethodHandlerFactory &other)
{
	(void)other;
	return *this;
}

MethodHandlerFactory::~MethodHandlerFactory()
{
	// Private destructor
}

IMethodHandler *MethodHandlerFactory::createHandler(const std::string &method)
{
	if (!_initialized)
	{
		initializeCreators();
	}

	std::map<std::string, HandlerCreator>::iterator it = _handlerCreators.find(method);
	if (it != _handlerCreators.end())
	{
		return it->second();
	}

	Logger::warning("MethodHandlerFactory: Unsupported method: " + method, __FILE__, __LINE__, __PRETTY_FUNCTION__);
	return NULL;
}

bool MethodHandlerFactory::isMethodSupported(const std::string &method)
{
	if (!_initialized)
	{
		initializeCreators();
	}

	return _handlerCreators.find(method) != _handlerCreators.end();
}

std::vector<std::string> MethodHandlerFactory::getSupportedMethods()
{
	if (!_initialized)
	{
		initializeCreators();
	}

	std::vector<std::string> methods;
	for (std::map<std::string, HandlerCreator>::const_iterator it = _handlerCreators.begin();
		 it != _handlerCreators.end(); ++it)
	{
		methods.push_back(it->first);
	}
	return methods;
}

void MethodHandlerFactory::initializeCreators()
{
	if (_initialized)
		return;

	_handlerCreators["GET"] = &createGetHandler;
	_handlerCreators["POST"] = &createPostHandler;
	_handlerCreators["DELETE"] = &createDeleteHandler;
	_handlerCreators["PUT"] = &createPutHandler;

	_initialized = true;
	Logger::debug("MethodHandlerFactory: Initialized with " + StrUtils::toString(_handlerCreators.size()) +
					  " handler creators",
				  __FILE__, __LINE__, __PRETTY_FUNCTION__);
}

// Helper functions for creating specific handlers
IMethodHandler *MethodHandlerFactory::createGetHandler()
{
	return new GetMethodHandler();
}

IMethodHandler *MethodHandlerFactory::createPostHandler()
{
	return new PostMethodHandler();
}

IMethodHandler *MethodHandlerFactory::createDeleteHandler()
{
	return new DeleteMethodHandler();
}

IMethodHandler *MethodHandlerFactory::createPutHandler()
{
	return new PutMethodHandler();
}
