#include "../../includes/Config/ConfigFileReader.hpp"
#include <stdexcept>
#include <string>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ConfigFileReader::ConfigFileReader(const std::string &filepath) : _ifs(filepath.c_str()), _lineNumber(0)
{
	if (!_ifs.is_open())
		throw std::runtime_error("Could not open config file: " + filepath);
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ConfigFileReader::~ConfigFileReader()
{
}

/*
** --------------------------------- METHODS ----------------------------------
*/

bool ConfigFileReader::nextLine(std::string &outLine)
{
	if (!std::getline(_ifs, outLine))
		return false;
	++_lineNumber;
	return true;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

int ConfigFileReader::getLineNumber() const
{
	return _lineNumber;
}

bool ConfigFileReader::isEof() const
{
	return _ifs.eof();
}

/* ************************************************************************** */
