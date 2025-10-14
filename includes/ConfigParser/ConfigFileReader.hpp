/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigFileReader.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: malee <malee@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/15 00:30:00 by malee             #+#    #+#             */
/*   Updated: 2025/10/15 00:31:54 by malee            ###   ########.fr       */
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

    // Non-copyable
    ConfigFileReader(const ConfigFileReader &src);
    ConfigFileReader &operator=(const ConfigFileReader &rhs);

public:
    ConfigFileReader();
    ConfigFileReader(const std::string &filename);
    ~ConfigFileReader();

    // Public methods
    bool nextLine(std::string &line);

    // Accessors
    bool isEof() const;
    int getLineNumber() const;
};
#endif /* ************************************************** CONFIGFILEREADER_H */