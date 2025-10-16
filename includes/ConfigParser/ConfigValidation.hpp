#ifndef CONFIGVALIDATION_HPP
#define CONFIGVALIDATION_HPP

#include <iostream>
#include <string>
#include "ConfigNameSpace.hpp"
#include "../Global/Logger.hpp"

// Validates the AST ignores directives and locations with errors by removing them from the AST
class ConfigValidation
{
private:
	AST::Config *_cfg; // non-owning borrowed pointer

	// Non copyable
	ConfigValidation(ConfigValidation const &);
	ConfigValidation &operator=(ConfigValidation const &);

	// Helpers
	void validateServerBlock(AST::Server &server);
	bool validateDirective(AST::Directive &directive);
	bool validateLocation(AST::Location &location);

	// Specific directive validators
	bool validateListenDirective(AST::Directive &directive);
	bool validateServerNameDirective(AST::Directive &directive);
	bool validateRootDirective(AST::Directive &directive);
	bool validateIndexDirective(AST::Directive &directive);
	bool validateAutoindexDirective(AST::Directive &directive);
	bool validateClientMaxUriSizeDirective(AST::Directive &directive);
	bool validateClientMaxHeadersSizeDirective(AST::Directive &directive);
	bool validateClientMaxBodySizeDirective(AST::Directive &directive);
	bool validateErrorPageDirective(AST::Directive &directive);
	bool validateKeepAliveDirective(AST::Directive &directive);
	bool validateAllowedMethodsDirective(AST::Directive &directive);
	bool validateReturnDirective(AST::Directive &directive);
	bool validateCgiPathDirective(AST::Directive &directive);
	bool validateCgiParamDirective(AST::Directive &directive);

	// Helper validation methods
	bool isValidPort(const std::string &port);
	bool isValidStatusCode(const std::string &code);
	bool validateSizeArgument(const std::string &arg, const std::string &directiveName, const AST::Directive &directive);

	// Enhanced validation methods
	void validateServerUniqueness();
	void validateLocationUniqueness(AST::Server &server);
	void validateLocationConflicts(AST::Location &location);

public:
	explicit ConfigValidation(AST::Config &cfg);
	~ConfigValidation();

	void validate();
};

#endif /* ************************************************ CONFIGVALIDATION_H */