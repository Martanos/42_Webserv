#ifndef CONFIGUTILS_HPP
#define CONFIGUTILS_HPP

#include "Logger.hpp"
#include "StringUtils.hpp"
#include <sstream>
#include <string>
#include <cctype>

// Utility class for configuration parsing operations
class ConfigUtils
{
private:
	ConfigUtils();
	ConfigUtils(ConfigUtils const &src);
	~ConfigUtils();
	ConfigUtils &operator=(ConfigUtils const &rhs);

public:
	// Error handling utilities
	static void throwConfigError(const std::string& msg, const char* file, int line)
	{
		Logger::log(Logger::ERROR, msg, file, line);
		throw std::runtime_error(msg);
	}

	// Validation utilities
	static bool validateDirective(std::stringstream& stream, const std::string& expectedDirective, double lineNumber, const char* file, int line)
	{
		std::string token;
		if (!(stream >> token) || token != expectedDirective)
		{
			std::stringstream ss;
			ss << "Invalid " << expectedDirective << " directive at line " << lineNumber;
			throwConfigError(ss.str(), file, line);
		}
		return true;
	}

	static void lineValidation(std::string &line, int lineNumber)
	{
		if (line.empty())
		{
			throwConfigError("Empty directive at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
		}
		else if (line.find_last_of(';') != line.length() - 1)
		{
			throwConfigError("Expected semicolon at the end of line " + StringUtils::toString(lineNumber) + " : " + line, __FILE__, __LINE__);
		}
		line.erase(line.find_last_of(';'));
	}

	// Size parsing utility
	static double parseSizeValue(std::stringstream& sizeStream, const std::string& directive, double lineNumber)
	{
		std::string valueToken;
		sizeStream >> valueToken; // Read the size value
		if (valueToken.empty()) {
			throwConfigError("Empty " + directive + " at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
		}
		char lastChar = valueToken[valueToken.length() - 1];
		double multiplier = 1.0;
		if (lastChar == 'k' || lastChar == 'K') {
			multiplier = 1024.0;
			valueToken.erase(valueToken.length() - 1);
		} else if (lastChar == 'm' || lastChar == 'M') {
			multiplier = 1024.0 * 1024.0;
			valueToken.erase(valueToken.length() - 1);
		} else if (lastChar == 'g' || lastChar == 'G') {
			multiplier = 1024.0 * 1024.0 * 1024.0;
			valueToken.erase(valueToken.length() - 1);
		} else if (!std::isdigit(lastChar)) {
			throwConfigError("Invalid suffix for " + directive + ": " + lastChar + " at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
		}
		// If lastChar is digit, no suffix, multiplier remains 1
		std::stringstream ss(valueToken);
		double size = 0.0;
		if (!(ss >> size)) {
			throwConfigError("Invalid " + directive + " value at line " + StringUtils::toString(lineNumber), __FILE__, __LINE__);
		}
		return size * multiplier;
	}
};

#endif /* CONFIGUTILS_HPP */
