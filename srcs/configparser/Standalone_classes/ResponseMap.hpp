#ifndef RESPONSEMAP_HPP
# define RESPONSEMAP_HPP

# include <iostream>
# include <string>

class ResponseMap
{

	public:

		ResponseMap();
		ResponseMap( ResponseMap const & src );
		~ResponseMap();

		ResponseMap &		operator=( ResponseMap const & rhs );

	private:

};

std::ostream &			operator<<( std::ostream & o, ResponseMap const & i );

#endif /* ***************************************************** RESPONSEMAP_H */