/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: malee <malee@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/24 18:16:26 by malee             #+#    #+#             */
/*   Updated: 2025/08/03 20:38:55 by malee            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "Logger.hpp"

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		Logger::log(Logger::ERROR, "Usage: " + std::string(argv[0]) + " <config_file>");
		return 1;
	}

	// Pseudo code structure
	// 1. Parse the config file
	ConfigParser parser(argv[1]);

	// const std::vector<ServerConfig>& servers = parser.getServerConfigs();
	// for (size_t i = 0; i < servers.size(); ++i) {
	//     const std::vector<int>& ports = servers[i].getPorts();
	//     for (size_t j = 0; j < ports.size(); ++j) {
	//         std::cout << "Server listening on port: " << ports[j] << std::endl;
	//     }
	// }

	parser.printAllConfigs();
	// 2. Setup the listening sockets
	// 3. Setup the poll_fds
	// 4. Wait for events
	// 5. Handle the events
	// 6. Close the sockets
	// 7. Exit
}
