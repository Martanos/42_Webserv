#include "../../includes/Config/ServerMap.hpp"
#include "../../includes/Global/Logger.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ServerMap::ServerMap(std::vector<Server> &servers)
{
	_servers = servers;
	_buildServerMap();
}

ServerMap::ServerMap(const ServerMap &src) : _serverMap(src._serverMap)
{
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ServerMap::~ServerMap()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

ServerMap &ServerMap::operator=(ServerMap const &rhs)
{
	if (this != &rhs)
	{
		_servers = rhs._servers;
		_serverMap = rhs._serverMap;
	}
	return *this;
}

/* --------------------------------- Private Utilities --------------------------------- */

void ServerMap::_buildServerMap()
{
	for (std::vector<Server>::iterator server_it = _servers.begin(); server_it != _servers.end(); ++server_it)
	{
		for (std::vector<SocketAddress>::const_iterator socketAddress_it = server_it->getSocketAddresses().begin();
			 socketAddress_it != server_it->getSocketAddresses().end(); ++socketAddress_it)
		{
			bool found = false;
			for (std::map<ListeningSocket, std::vector<Server> >::iterator listening_socket_it = _serverMap.begin();
				 listening_socket_it != _serverMap.end(); ++listening_socket_it)
			{
				if (listening_socket_it->first.getAddress() == *socketAddress_it)
				{
					listening_socket_it->second.push_back(*server_it);
					found = true;
					break;
				}
			}
			if (!found)
			{
				try
				{
					ListeningSocket listeningSocket(*socketAddress_it);
					listeningSocket.bind();
					listeningSocket.listen();
					_serverMap.insert(std::make_pair(listeningSocket, std::vector<Server>(1, *server_it)));
				}
				catch (const std::exception &e)
				{
					Logger::warning("ServerMap: Error adding listening socket [" + socketAddress_it->getPortString() +
										"]: " + std::string(e.what()),
									__FILE__, __LINE__, __PRETTY_FUNCTION__);
					continue;
				}
			}
		}
	}
}

bool ServerMap::hasFd(int &fd) const
{
	for (std::map<ListeningSocket, std::vector<Server> >::const_iterator it = _serverMap.begin();
		 it != _serverMap.end(); ++it)
	{
		if (it->first.getFd() == fd)
			return true;
	}
	return false;
}

const ListeningSocket &ServerMap::getListeningSocket(int &fd) const
{
	for (std::map<ListeningSocket, std::vector<Server> >::const_iterator it = _serverMap.begin();
		 it != _serverMap.end(); ++it)
	{
		if (it->first.getFd() == fd)
			return it->first;
	}
	throw std::out_of_range("ServerMap: Listening socket not found");
}

const std::map<ListeningSocket, std::vector<Server> > &ServerMap::getServerMap() const
{
	return _serverMap;
}

void ServerMap::printServerMap() const
{
	for (std::map<ListeningSocket, std::vector<Server> >::const_iterator it = _serverMap.begin();
		 it != _serverMap.end(); ++it)
	{
		printf("ServerMap: Listening socket: fd: %d, host: %s, port: %d, memory address: %p\n",
			   it->first.getFd().getFd(), it->first.getAddress().getHostString().c_str(),
			   it->first.getAddress().getPort(), &it->first);
		printf("ServerMap: Servers: %zu\n", it->second.size());
		for (std::vector<Server>::const_iterator server = it->second.begin(); server != it->second.end(); ++server)
		{
			for (TrieTree<std::string>::const_iterator serverName = server->getServerNames().begin();
				 serverName != server->getServerNames().end(); ++serverName)
			{
				printf("ServerMap: Server name: %s\n", serverName->c_str());
			}
		}
		printf("ServerMap: End of listening socket\n");
	}
}

const std::vector<Server> &ServerMap::getServersForFd(int fd) const
{
	for (std::map<ListeningSocket, std::vector<Server> >::const_iterator it = _serverMap.begin();
		 it != _serverMap.end(); ++it)
	{
		if (it->first.getFd().getFd() == fd)
		{
			return it->second;
		}
	}
	static std::vector<Server> empty;
	return empty;
}

bool ServerMap::empty() const
{
	return _serverMap.empty();
}

/* ************************************************************************** */
