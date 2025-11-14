#ifndef DIRECTIVES_HPP
#define DIRECTIVES_HPP

#include "../../includes/Wrapper/TrieTree.hpp"
#include <map>
#include <string>
#include <utility>
#include <vector>

// Abstract class for directive-related functionality
// Base class for directive-related functionality. Derived classes (Server/Location)
// access directive storage and flags through protected members below.
class Directives
{
protected:
	// Directive members
	std::string _rootPath;
	bool _autoIndexValue;
	std::string _cgiPath;
	double _clientMaxBodySize;
	bool _keepAliveValue;
	std::pair<int, std::string> _redirect;
	TrieTree<std::string> _indexes;
	std::map<int, std::string> _statusPaths;
	std::vector<std::string> _allowedMethods;

	// Directive flags
	bool _hasRootPathDirective;
	bool _hasAutoIndexDirective;
	bool _hasCgiPathDirective;
	bool _hasClientMaxBodySizeDirective;
	bool _hasKeepAliveDirective;
	bool _hasRedirectDirective;
	bool _hasIndexDirective;
	bool _hasStatusPathDirective;
	bool _hasAllowedMethodsDirective;

public:
	// Default constructor initializes directive storage and flags
	Directives();
	virtual ~Directives();

	// Directive flags
	bool hasRootPathDirective() const;
	bool hasAutoIndexDirective() const;
	bool hasCgiPathDirective() const;
	bool hasClientMaxBodySizeDirective() const;
	bool hasKeepAliveDirective() const;
	bool hasRedirectDirective() const;
	bool hasIndexDirective() const;
	bool hasStatusPathDirective() const;
	bool hasAllowedMethodsDirective() const;

	// Directive Investigators
	bool hasIndex(const std::string &index) const;
	bool hasStatusPath(int status) const;
	bool hasAllowedMethod(const std::string &allowedMethod) const;

	// Directive Accessors
	const std::string &getRootPath() const;
	bool getAutoIndexValue() const;
	const std::string &getCgiPath() const;
	double getClientMaxBodySize() const;
	bool getKeepAliveValue() const;
	const std::pair<int, std::string> &getRedirect() const;
	const TrieTree<std::string> &getIndexes() const;
	const std::string &getStatusPath(int status) const;
	const std::map<int, std::string> &getStatusPaths() const;
	const std::vector<std::string> &getAllowedMethods() const;

	// Directive Mutator
	void setRootPath(const std::string &root);
	void setAutoIndex(bool autoIndex);
	void setCgiPath(const std::string &cgiPath);
	void setClientMaxBodySize(double clientMaxBodySize);
	void setKeepAlive(bool keepAlive);
	void setRedirect(const std::pair<int, std::string> &redirect);
	void insertIndex(const std::string &index);
	void setIndexes(const TrieTree<std::string> &indexes);
	void insertStatusPath(const std::vector<int> &codes, const std::string &path);
	void setStatusPaths(const std::map<int, std::string> &statusPages);
	void insertAllowedMethod(const std::string &allowedMethod);
	void setAllowedMethods(const std::vector<std::string> &allowedMethods);
};

#endif
