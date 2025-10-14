#ifndef STRINGUTILS_HPP
#define STRINGUTILS_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>

// Static class for string operations
// TODO:Move Utils to namespace
class StringUtils
{
private:
	StringUtils();
	StringUtils(StringUtils const &src);
	~StringUtils();
	StringUtils &operator=(StringUtils const &rhs);

public:
	static std::string toLowerCase(const std::string &str)
	{
		std::string result = str;
		std::transform(result.begin(), result.end(), result.begin(), ::tolower);
		return result;
	}

	static std::string toUpperCase(const std::string &str)
	{
		std::string result = str;
		std::transform(result.begin(), result.end(), result.begin(), ::toupper);
		return result;
	}

	static std::string toString(size_t value)
	{
		std::stringstream ss;
		ss << value;
		return ss.str();
	}
};

std::ostream &operator<<(std::ostream &o, StringUtils const &i);

#endif /* ***************************************************** STRINGUTILS_H                                          \
		*/
