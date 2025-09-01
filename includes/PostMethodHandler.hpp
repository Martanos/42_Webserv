#ifndef POSTMETHODHANDLER_HPP
# define POSTMETHODHANDLER_HPP

# include <iostream>
# include <string>

class PostMethodHandler
{

	public:

		PostMethodHandler();
		PostMethodHandler( PostMethodHandler const & src );
		~PostMethodHandler();

		PostMethodHandler &		operator=( PostMethodHandler const & rhs );

	private:

};

std::ostream &			operator<<( std::ostream & o, PostMethodHandler const & i );

#endif /* *********************************************** POSTMETHODHANDLER_H */