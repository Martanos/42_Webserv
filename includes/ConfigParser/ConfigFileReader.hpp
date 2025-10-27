#ifndef CONFIGFILEREADER_HPP
#define CONFIGFILEREADER_HPP

#include <fstream>
#include <string>

class ConfigFileReader
{
private:
	std::ifstream _ifs;
	int _lineNumber;

	// Non copyable
	ConfigFileReader(const ConfigFileReader &src);
	ConfigFileReader &operator=(const ConfigFileReader &rhs);

public:
	explicit ConfigFileReader(const std::string &filepath);
	~ConfigFileReader();

	// Public methods
	bool nextLine(std::string &line);

	// Accessors
	bool isEof() const;
	int getLineNumber() const;
};
#endif /* ************************************************** CONFIGFILEREADER_H */