#include "../../includes/ConfigParser/ServerMap.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"

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
				_serverMap.insert(
					std::make_pair(ListeningSocket(*socketAddress_it), std::vector<Server>(1, *server_it)));
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
	std::stringstream ss;
	for (std::map<ListeningSocket, std::vector<Server> >::const_iterator it = _serverMap.begin();
		 it != _serverMap.end(); ++it)
	{
		ss << "ServerMap: Listening socket: fd: " << it->first.getFd()
		   << ", host: " << it->first.getAddress().getHostString() << ", port: " << it->first.getAddress().getPort()
		   << " memory address: " << &it->first << std::endl;
		ss << "Servers:" << std::endl;
		for (std::vector<Server>::const_iterator server = it->second.begin(); server != it->second.end(); ++server)
		{
		ss << " Server Name(s):";
		if (!server->getServerNames().isEmpty())
		{
			for (TrieTree<std::string>::const_iterator serverName = server->getServerNames().begin();
				 serverName != server->getServerNames().end(); ++serverName)
			{
				ss << " " << *serverName << std::endl;
			}
		}
		else
		{
			ss << " (none)" << std::endl;
		}
		}
	}
	Logger::debug(ss.str());
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

/* ************************************************************************** */
