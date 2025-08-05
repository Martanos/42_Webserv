#ifndef SERVERSTITCHER_HPP
#define SERVERSTITCHER_HPP

#include <iostream>
#include <string>

// TODO:
// This class is responsible for combining serveral maps
// Maps Serverkey to ServerObjects
// Sifts out duplicates logging them as errors
// Attaches Mimetypes map and ResponseMap references to the ServerObject
// Outputs a map of Serverkey to ServerObjects

class ServerStitcher
{

public:
	ServerStitcher();
	ServerStitcher(ServerStitcher const &src);
	~ServerStitcher();

	ServerStitcher &operator=(ServerStitcher const &rhs);

private:
};

std::ostream &operator<<(std::ostream &o, ServerStitcher const &i);

#endif /* ************************************************** SERVERSTITCHER_H */
