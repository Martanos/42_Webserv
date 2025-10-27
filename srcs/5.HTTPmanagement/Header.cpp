#include "../../includes/HTTP/Header.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include <cstddef>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Header::Header()
{
	_directive = "";
	_values = std::vector<std::string>();
	_parameters = std::vector<std::pair<std::string, std::string> >();
	_rawHeader = "";
}

Header::Header(const std::string &rawHeader)
{
	_rawHeader = rawHeader;
	_parseRawHeader();
}

Header::Header(const Header &other)
{
	*this = other;
}

Header &Header::operator=(const Header &other)
{
	if (this != &other)
	{
		_directive = other._directive;
		_values = other._values;
		_parameters = other._parameters;
		_rawHeader = other._rawHeader;
	}
	return *this;
}

Header::~Header()
{
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void Header::_parseRawHeader()
{
	// Break down the raw header into directive, values, and parameters

	if (_rawHeader.empty())
		throw std::invalid_argument("Empty raw header");
	// Directive extraction and validation and normalization
	size_t colonPos = _rawHeader.find(':');
	if (colonPos == std::string::npos)
		throw std::invalid_argument("No colon found in raw header");
	_directive = _rawHeader.substr(0, colonPos);
	if (!StrUtils::isValidToken(_directive))
		throw std::invalid_argument("Directive is not a valid token");
	_directive = StrUtils::toLowerCase(_directive);

	// Extract raw values by iterating through till first ; or end of string
	size_t valuesStart = colonPos + 1;
	size_t valuesEnd = _rawHeader.find_first_of(";");

	// If no ; is found, set valuesEnd to end of string
	bool hasParameters = true;
	if (valuesEnd == std::string::npos)
	{
		valuesEnd = _rawHeader.length();
		hasParameters = false;
	}

	std::string rawValues = _rawHeader.substr(valuesStart, valuesEnd - valuesStart);
	if (rawValues.empty())
		throw std::invalid_argument("Empty values");
	_values = StrUtils::splitString(rawValues, ',');
	for (size_t i = 0; i < _values.size(); i++)
	{
		// Verify that value is not empty
		if (_values[i].empty())
			throw std::invalid_argument("Empty value");
		// Trim token of any intial valid whitespace
		_values[i] = StrUtils::trimLeadingSpaces(_values[i]);
		// Verify that value is a valid token
		if (!StrUtils::isValidToken(_values[i]))
			throw std::invalid_argument("Value is not a valid token");
		// Normalize value to lowercase
		_values[i] = StrUtils::toLowerCase(_values[i]);
	}

	if (hasParameters)
	{
		// Extract parameters by iterating through till end of string
		size_t parametersStart = valuesEnd + 1;
		size_t parametersEnd = _rawHeader.length();
		std::string rawParameters = _rawHeader.substr(parametersStart, parametersEnd - parametersStart);
		if (rawParameters.empty())
			throw std::invalid_argument("Empty parameters");
		std::vector<std::string> parameters = StrUtils::splitString(rawParameters, ';');
		for (size_t i = 0; i < parameters.size(); i++)
		{
			if (parameters[i].empty())
				throw std::invalid_argument("Empty parameter");
			parameters[i] = StrUtils::trimLeadingSpaces(parameters[i]);

			size_t equalSignPos = parameters[i].find('=');
			if (equalSignPos == std::string::npos)
				throw std::invalid_argument("No equal sign found in parameter");

			std::string key = parameters[i].substr(0, equalSignPos);
			if (!StrUtils::isValidToken(key))
				throw std::invalid_argument("Parameter key is not a valid token");
			key = StrUtils::toLowerCase(key);

			std::string value = parameters[i].substr(equalSignPos + 1);
			if (value.empty())
				throw std::invalid_argument("Empty parameter value");
			else if (value[0] == '\"' && value[value.length() - 1] == '\"')
			{
				value = value.substr(1, value.length() - 2);
				value = StrUtils::percentDecode(value);
				if (!StrUtils::hasControlCharacters(value))
					throw std::invalid_argument("Parameter value has control characters");
			}
			else if (!StrUtils::isValidToken(value))
				throw std::invalid_argument("Parameter value is not a valid token");
			_parameters.push_back(std::make_pair(key, value));
		}
	}
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void Header::merge(const Header &other)
{
	for (size_t i = 0; i < other._values.size(); i++)
	{
		if (std::find(_values.begin(), _values.end(), other._values[i]) == _values.end())
			_values.push_back(other._values[i]);
	}
	for (size_t i = 0; i < other._parameters.size(); i++)
	{
		if (std::find(_parameters.begin(), _parameters.end(), other._parameters[i]) == _parameters.end())
			_parameters.push_back(other._parameters[i]);
	}
}

/*
** --------------------------------- COMPARATORS ---------------------------------
*/

bool Header::operator==(const Header &other) const
{
	return _directive == other._directive;
}

bool Header::operator!=(const Header &other) const
{
	return _directive != other._directive;
}

bool Header::operator<(const Header &other) const
{
	return _directive < other._directive;
}

bool Header::operator>(const Header &other) const
{
	return _directive > other._directive;
}

bool Header::operator<=(const Header &other) const
{
	return _directive <= other._directive;
}

bool Header::operator>=(const Header &other) const
{
	return _directive >= other._directive;
}

/*
** --------------------------------- ACCESSORS ---------------------------------
*/

const std::string &Header::getDirective() const
{
	return _directive;
}

const std::vector<std::string> &Header::getValues() const
{
	return _values;
}

const std::vector<std::pair<std::string, std::string> > &Header::getParameters() const
{
	return _parameters;
}

const std::string &Header::getRawHeader() const
{
	return _rawHeader;
}