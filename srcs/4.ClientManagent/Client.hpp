#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <iostream>
# include <string>

class Client
{

	public:

		Client();
		Client( Client const & src );
		~Client();

		Client &		operator=( Client const & rhs );

	private:

};

std::ostream &			operator<<( std::ostream & o, Client const & i );

#endif /* ********************************************************** CLIENT_H */