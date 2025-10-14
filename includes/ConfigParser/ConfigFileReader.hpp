/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigFileReader.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: malee <malee@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 00:30:00 by malee             #+#    #+#             */
/*   Updated: 2025/10/15 01:19:51 by malee            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGFILEREADER_HPP
#define CONFIGFILEREADER_HPP

#include <fstream>
#include <string>

class ConfigFileReader
{
private:
    std::ifstream file;
    int lineNumber;

public:
    ConfigFileReader();
    ConfigFileReader(const std::string &filename);
    ConfigFileReader(const ConfigFileReader &src);
    ~ConfigFileReader();

    ConfigFileReader &operator=(const ConfigFileReader &rhs);

    // Public methods
    bool nextLine(std::string &line);

    // Mutators
    void setFilename(const std::string &filename);

    // Accessors
    bool isEof() const;
    int getLineNumber() const;
};
#endif /* ************************************************** CONFIGFILEREADER_H */