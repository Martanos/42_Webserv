#ifndef MIMETYPES_HPP
#define MIMETYPES_HPP

#include <iostream>
#include <string>

// TODO:
// This class reads from system file /etc/mime.types
// Writes to a static map of mime types and their corresponding extensions
// Default mime types are used as a fallback
// Outputs a map of mime types and their corresponding extensions
class MimeTypes
{

public:
	MimeTypes();
	MimeTypes(MimeTypes const &src);
	~MimeTypes();

	MimeTypes &operator=(MimeTypes const &rhs);

private:
};

std::ostream &operator<<(std::ostream &o, MimeTypes const &i);

#endif /* ******************************************************* MIMETYPES_H */
