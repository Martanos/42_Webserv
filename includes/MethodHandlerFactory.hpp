#ifndef METHODHANDLERFACTORY_HPP
#define METHODHANDLERFACTORY_HPP

#include <map>
#include <string>
#include "IMethodHandler.hpp"

class MethodHandlerFactory
{
private:
	std::map<std::string, IMethodHandler *> _handlers;

	MethodHandlerFactory();
	~MethodHandlerFactory();
	MethodHandlerFactory(const MethodHandlerFactory &);
	MethodHandlerFactory &operator=(const MethodHandlerFactory &);

public:
	static MethodHandlerFactory &getInstance()
	{
		static MethodHandlerFactory instance;
		return instance;
	}

	IMethodHandler *getHandler(const std::string &method) const;
	bool isMethodSupported(const std::string &method) const;
	std::vector<std::string> getSupportedMethods() const;
};

#endif
