#ifndef ADDRINFO_HPP
# define ADDRINFO_HPP

# include <iostream>
# include <string>

class AddrInfo
{

	public:

		AddrInfo();
		AddrInfo( AddrInfo const & src );
		~AddrInfo();

		AddrInfo &		operator=( AddrInfo const & rhs );

	private:

};

std::ostream &			operator<<( std::ostream & o, AddrInfo const & i );

#endif /* ******************************************************** ADDRINFO_H */