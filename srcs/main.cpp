/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: malee <malee@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/24 18:16:26 by malee             #+#    #+#             */
/*   Updated: 2025/07/24 18:17:32 by malee            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		throw std::runtime_error("Invalid number of arguments");
	}
	// Pseudo code structure
	// 1. Parse the config file
	// 2. Setup the listening sockets
	// 3. Setup the poll_fds
	// 4. Wait for events
	// 5. Handle the events
	// 6. Close the sockets
	// 7. Exit
}
