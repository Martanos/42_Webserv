/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigFileReader.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: malee <malee@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 00:28:37 by malee             #+#    #+#             */
/*   Updated: 2025/10/15 00:32:35 by malee            ###   ########.fr       */
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
	throw std::runtime_error("ConfigFileReader is not copyable");
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
	throw std::runtime_error("ConfigFileReader is not assignable");
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

/* ************************************************************************** */