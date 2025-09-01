#ifndef FORMATTINGUTILS_HPP
# define FORMATTINGUTILS_HPP

# include <iostream>
# include <string>

class FormattingUtils
{

	public:

		FormattingUtils();
		FormattingUtils( FormattingUtils const & src );
		~FormattingUtils();

		FormattingUtils &		operator=( FormattingUtils const & rhs );

	private:

};

std::ostream &			operator<<( std::ostream & o, FormattingUtils const & i );

#endif /* ************************************************* FORMATTINGUTILS_H */