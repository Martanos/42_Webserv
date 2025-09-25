/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: malee <malee@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/24 18:16:26 by malee             #+#    #+#             */
/*   Updated: 2025/09/25 17:41:28 by malee            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/ConfigParser.hpp"
#include "../includes/Logger.hpp"
#include "../includes/ServerConfig.hpp"
#include "../includes/ServerManager.hpp"
#include "../includes/StringUtils.hpp"

int main(int argc, char **argv)
{
	// Initialize logger session
	Logger::initializeSession("logs");

	if (argc != 2)
	{
		Logger::log(Logger::ERROR, "Usage: " + std::string(argv[0]) + " <config_file>");
		Logger::closeSession();
		return 1;
	}

	try
	{
		Logger::log(Logger::INFO, "Starting WebServ with config file: " + std::string(argv[1]));

		// 1. Parse the config file
		ConfigParser parser(argv[1]);

		// Get server configurations
		const std::vector<ServerConfig> &servers = parser.getServerConfigs();
		if (servers.empty())
		{
			Logger::log(Logger::ERROR, "No server configurations found in config file");
			Logger::closeSession();
			return 1;
		}

		Logger::log(Logger::INFO, "Parsed " + StringUtils::toString(servers.size()) + " server configurations");

		// Print configuration for debugging
		parser.printAllConfigs();

		// 2. Create and run ServerManager
		ServerManager serverManager;

		// Convert const reference to non-const for ServerManager
		std::vector<ServerConfig> serverConfigs = servers;

		Logger::log(Logger::INFO, "Starting server manager...");

		// 3. Run the server (this handles all the socket setup, epoll, and event loop)
		serverManager.run(serverConfigs);

		Logger::log(Logger::INFO, "WebServ shutdown completed successfully");
	}
	catch (const std::exception &e)
	{
		Logger::log(Logger::ERROR, "WebServ failed: " + std::string(e.what()));
		Logger::closeSession();
		return 1;
	}

	Logger::closeSession();
	return 0;
}
