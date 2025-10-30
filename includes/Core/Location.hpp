#ifndef LOCATION_HPP
#define LOCATION_HPP

#include "../../includes/Wrapper/TrieTree.hpp"
#include <map>
#include <string>
#include <vector>

// Location configuration object
class Location
{
private:
	// Identifier members
	std::string _path;
	std::string _root;
	std::vector<std::string> _allowedMethods;
	TrieTree<std::string> _indexes;
	std::map<int, std::string> _statusPages;
	bool _autoIndex;
	std::pair<int, std::string> _redirect;
	std::string _cgiPath;
	double _clientMaxBodySize;
	std::map<std::string, std::string> _cgiParams;

	// Flags
	bool _modified;

public:
	explicit Location(const std::string &path);
	Location(Location const &src);
	~Location();
	Location &operator=(Location const &rhs);

	// Investigators
	bool hasAllowedMethod(const std::string &allowedMethod) const;
	bool hasStatusPage(const int &status) const;
	bool hasRedirect() const;
	bool hasAutoIndex() const;
	bool hasIndex(const std::string &index) const;
	bool hasIndexes() const;
	bool hasCgiPath() const;
	bool hasClientMaxBodySize() const;
	bool hasCgiParams() const;
	bool hasModified() const;
	bool hasRoot() const;

	// Accessors
	const std::string &getPath() const;
	const std::string &getRoot() const;
	const std::vector<std::string> &getAllowedMethods() const;
	const std::map<int, std::string> &getStatusPages() const;
	const std::pair<int, std::string> &getRedirect() const;
	const TrieTree<std::string> &getIndexes() const;
	const std::string &getCgiPath() const;
	double getClientMaxBodySize() const;
	const std::map<std::string, std::string> &getCgiParams() const;


	// Mutators
	void setPath(const std::string &path);
	void setRoot(const std::string &root);
	void insertIndex(const std::string &index);
	void insertAllowedMethod(const std::string &allowedMethod);
	void insertStatusPage(const std::vector<int> &codes, const std::string &path);
	void setRedirect(const std::pair<int, std::string> &redirect);
	void setAutoIndex(const bool &autoIndex);
	void setCgiPath(const std::string &cgiPath);
	void setClientMaxBodySize(double size);
	void setCgiParam(const std::string &key, const std::string &value);
};

void operator<<(std::ostream &o, Location const &i);

#endif /* ******************************************************** LOCATION_H                                          \
		*/
