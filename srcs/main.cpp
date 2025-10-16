/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: malee <malee@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/24 18:16:26 by malee             #+#    #+#             */
/*   Updated: 2025/10/16 10:36:09 by malee            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/ConfigParser/ConfigFileReader.hpp"
#include "../includes/ConfigParser/ConfigParser.hpp"
#include "../includes/ConfigParser/ConfigTokeniser.hpp"
#include "../includes/Global/Logger.hpp"
#include "../includes/Global/PerformanceMonitor.hpp"
#include "../includes/Core/Server.hpp"
#include "../includes/Core/ServerManager.hpp"
#include "../includes/Global/StringUtils.hpp"
#include "../includes/ConfigParser/ConfigNameSpace.hpp"

int main(int argc, char **argv)
{
	// Initialize logger session
	Logger::initializeSession("logs");

	// Initialize performance monitoring
	PerformanceMonitor &perfMonitor = PerformanceMonitor::getInstance();
	perfMonitor.setPerformanceThresholds(1000.0, 5000.0, 100 * 1024 * 1024); // 1s, 5s, 100MB
	Logger::info("PerformanceMonitor: Performance monitoring initialized", __FILE__, __LINE__);

	if (argc != 2)
	{
		Logger::log(Logger::ERROR, "Usage: " + std::string(argv[0]) + " <config_file>");
		Logger::closeSession();
		return 1;
	}

	try
	{
		Logger::log(Logger::INFO, "Starting WebServ with config file: " + std::string(argv[1]));

		// 1. Build the AST
		ConfigFileReader reader(argv[1]);
		ConfigTokeniser tokenizer(reader);
		ConfigParser parser(tokenizer);
		AST::Config cfg = parser.parse();
		if (cfg.servers.empty())
			throw std::runtime_error("No server blocks found in config file");
		parser.printAST(cfg); // Temporary for debugging

		// 2. Validate the AST
		// TODO: Validation Layer
		ConfigValidation validator(cfg);
		validator.validate();
		if (cfg.servers.empty())
			throw std::runtime_error("No valid server blocks found in config file");
		parser.printAST(cfg); // Print the AST after validation

		// 3. Map configuration
		// TODO: Transformation Layer
		// TODO: Binding layer
		// TODO: Mapping layer

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

		// Log final performance report
		perfMonitor.logPerformanceReport();
	}
	catch (const std::exception &e)
	{
		Logger::error("WebServ failed: " + std::string(e.what()), __FILE__, __LINE__);

		// Log performance report even on failure
		perfMonitor.logPerformanceSummary();

		Logger::closeSession();
		return 1;
	}

	// Cleanup performance monitoring
	PerformanceMonitor::destroyInstance();
	Logger::closeSession();
	return 0;
}
