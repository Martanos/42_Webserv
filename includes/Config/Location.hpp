#ifndef LOCATION_HPP
#define LOCATION_HPP

#include "../../includes/Config/Directives.hpp"
#include <string>

// Location configuration object
// Basically a smaller server block with path matching
class Location : public Directives
{
private:
	std::string _locationPath;

public:
	explicit Location(const std::string &path);
	Location(Location const &src);
	~Location();
	Location &operator=(Location const &rhs);

	// Identifier accessors
	const std::string &getLocationPath() const;

	// Utility
	bool wasModified() const;
	void reset();
};

std::ostream &operator<<(std::ostream &o, Location const &i);

#endif /* ******************************************************** LOCATION_H                                          \
		*/
