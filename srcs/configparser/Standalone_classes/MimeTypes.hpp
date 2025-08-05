#ifndef MIMETYPES_HPP
# define MIMETYPES_HPP

# include <iostream>
# include <string>

class MimeTypes
{

	public:

		MimeTypes();
		MimeTypes( MimeTypes const & src );
		~MimeTypes();

		MimeTypes &		operator=( MimeTypes const & rhs );

	private:

};

std::ostream &			operator<<( std::ostream & o, MimeTypes const & i );

#endif /* ******************************************************* MIMETYPES_H */