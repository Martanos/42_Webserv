#ifndef RESPONSEMAP_HPP
#define RESPONSEMAP_HPP

#include <iostream>
#include <string>

// TODO:
// This class reads from a file predefined response codes and their corresponding pages
// Writes to a static map of response codes and their corresponding pages
// Default response Map is used as a fallback
class ResponseMap
{

public:
	ResponseMap();
	ResponseMap(ResponseMap const &src);
	~ResponseMap();

	ResponseMap &operator=(ResponseMap const &rhs);

private:
};

std::ostream &operator<<(std::ostream &o, ResponseMap const &i);

#endif /* ***************************************************** RESPONSEMAP_H */
