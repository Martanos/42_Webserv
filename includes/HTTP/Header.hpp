#ifndef HEADER_HPP
#define HEADER_HPP

#include <string>
#include <vector>

// Represents a header and its accompanying functions
class Header
{
private:
	// Header directive
	std::string _directive;
	// Header values
	std::vector<std::string> _values;
	// Header parameters
	std::vector<std::pair<std::string, std::string> > _parameters;
	// Raw header string
	std::string _rawHeader;

	// Helper methods
	void _parseRawHeader();

public:
	Header();
	Header(const std::string &rawHeader);
	Header(const Header &other);
	Header &operator=(const Header &other);
	~Header();

	// Comparator overloads
	bool operator==(const Header &other) const;
	bool operator!=(const Header &other) const;
	bool operator<(const Header &other) const;
	bool operator>(const Header &other) const;
	bool operator<=(const Header &other) const;
	bool operator>=(const Header &other) const;

	// Accessors
	const std::string &getDirective() const;
	const std::vector<std::string> &getValues() const;
	const std::vector<std::pair<std::string, std::string> > &getParameters() const;
	const std::string &getRawHeader() const;

	// Methods
	void merge(const Header &other);
};

#endif /* HEADER_HPP */