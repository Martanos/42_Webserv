#ifndef STRINGUTILS_HPP
#define STRINGUTILS_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <string.h>
#include <string>
#include <vector>

namespace StrUtils
{

std::string toLowerCase(const std::string &str)
{
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

std::string toUpperCase(const std::string &str)
{
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);
	return result;
}

template <typename T> std::string toString(T value)
{
	std::stringstream ss;
	ss << value;
	return ss.str();
}

template <typename T> T fromString(const std::string &str)
{
	T value;
	std::stringstream ss(str);
	ss >> value;
	return value;
}

std::vector<std::string> splitString(const std::string &str, char delimiter)
{
	std::vector<std::string> parts;
	std::string current;

	for (size_t i = 0; i < str.length(); ++i)
	{
		if (str[i] == delimiter)
		{
			parts.push_back(current);
			current.clear();
		}
		else
		{
			current += str[i];
		}
	}
	parts.push_back(current); // Add the last part
	return parts;
}


} // namespace StrUtils

#endif // STRINGUTILS_HPP
