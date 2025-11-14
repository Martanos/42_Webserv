#ifndef METHODHANDLERFACTORY_HPP
#define METHODHANDLERFACTORY_HPP

#include "IMethodHandler.hpp"
#include <map>
#include <string>

class MethodHandlerFactory
{
public:
	// Static method to create appropriate handler
	static IMethodHandler *createHandler(const std::string &method);

	// Static method to check if method is supported
	static bool isMethodSupported(const std::string &method);

	// Static method to get list of supported methods
	static std::vector<std::string> getSupportedMethods();

private:
	// Private constructor - this is a utility class
	MethodHandlerFactory();
	MethodHandlerFactory(const MethodHandlerFactory &other);
	MethodHandlerFactory &operator=(const MethodHandlerFactory &other);
	~MethodHandlerFactory();

	// Static map of method to handler creation functions
	typedef IMethodHandler *(*HandlerCreator)();
	static std::map<std::string, HandlerCreator> _handlerCreators;
	static bool _initialized;
	static void initializeCreators();

	// Helper functions for creating specific handlers
	static IMethodHandler *createGetHandler();
	static IMethodHandler *createPostHandler();
	static IMethodHandler *createDeleteHandler();
	static IMethodHandler *createPutHandler();
};

#endif /* METHODHANDLERFACTORY_HPP */
