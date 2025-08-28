#ifndef REQUESTROUTING_HPP
#define REQUESTROUTING_HPP

#include <iostream>
#include <string>

// TODO: Implement this
// This should be a static util class that helps with parsed request routing
class RequestRouting
{

public:
	RequestRouting();
	RequestRouting(RequestRouting const &src);
	~RequestRouting();

	RequestRouting &operator=(RequestRouting const &rhs);

private:
};

std::ostream &operator<<(std::ostream &o, RequestRouting const &i);

#endif /* ************************************************** REQUESTROUTING_H */
