/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigFileReader.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: malee <malee@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 00:28:37 by malee             #+#    #+#             */
/*   Updated: 2025/10/15 01:21:09 by malee            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/ConfigParser/ConfigFileReader.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ConfigFileReader::ConfigFileReader() : file(), lineNumber(0)
{
}

ConfigFileReader::ConfigFileReader(const ConfigFileReader &src)
{
	*this = src;
}

ConfigFileReader::ConfigFileReader(const std::string &path)
	: file(path.c_str()), lineNumber(0)
{
	if (!file.is_open())
		throw std::runtime_error("Could not open config file: " + path);
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ConfigFileReader::~ConfigFileReader()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ConfigFileReader &ConfigFileReader::operator=(ConfigFileReader const &rhs)
{
	if (this != &rhs)
	{

		lineNumber = rhs.lineNumber;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

bool ConfigFileReader::nextLine(std::string &outLine)
{
	if (!std::getline(file, outLine))
		return false;
	++lineNumber;
	return true;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

int ConfigFileReader::getLineNumber() const
{
	return lineNumber;
}

bool ConfigFileReader::isEof() const
{
	return file.eof();
}

void ConfigFileReader::setFilename(const std::string &filename)
{
	file.close();
	file.open(filename.c_str());
	if (!file.is_open())
		throw std::runtime_error("Could not open config file: " + filename);
}

/* ************************************************************************** */