#ifndef LISTENINGSOCKET_HPP
# define LISTENINGSOCKET_HPP

# include <iostream>
# include <string>

class ListeningSocket
{

	public:

		ListeningSocket();
		ListeningSocket( ListeningSocket const & src );
		~ListeningSocket();

		ListeningSocket &		operator=( ListeningSocket const & rhs );

	private:

};

std::ostream &			operator<<( std::ostream & o, ListeningSocket const & i );

#endif /* ************************************************* LISTENINGSOCKET_H */