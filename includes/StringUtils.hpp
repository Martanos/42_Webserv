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
#include <vector>

// Static class for string operations
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

	static std::string toString(int value)
	{
		std::stringstream ss;
		ss << value;
		return ss.str();
	}

	static std::string toString(ssize_t value)
	{
		std::stringstream ss;
		ss << value;
		return ss.str();
	}

	static std::string toString(double number)
	{
		std::stringstream ss;
		ss << number;
		return ss.str();
	}

	static std::string trim(const std::string &str)
	{
		// First, remove inline comments
		std::string cleaned = str;
		size_t commentPos = cleaned.find('#');
		if (commentPos != std::string::npos)
		{
			cleaned = cleaned.substr(0, commentPos);
		}

		size_t first = cleaned.find_first_not_of(" \t\n\r");
		if (first == std::string::npos)
			return ""; // No non-whitespace characters
		size_t last = cleaned.find_last_not_of(" \t\n\r");
		return cleaned.substr(first, (last - first + 1));
	}

	static std::vector<std::string> split(const std::string &str)
	{
		std::vector<std::string> tokens;
		std::istringstream stream(str);
		std::string token;

		while (stream >> token)
		{
			tokens.push_back(token);
		}

		return tokens;
	}
};

std::ostream &operator<<(std::ostream &o, StringUtils const &i);

#endif /* ***************************************************** STRINGUTILS_H                                          \
		*/
