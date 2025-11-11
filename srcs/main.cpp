#include "../includes/ConfigParser/ConfigFileReader.hpp"
#include "../includes/ConfigParser/ConfigNameSpace.hpp"
#include "../includes/ConfigParser/ConfigParser.hpp"
#include "../includes/ConfigParser/ConfigTokeniser.hpp"
#include "../includes/ConfigParser/ConfigTranslator.hpp"
#include "../includes/ConfigParser/ServerMap.hpp"
#include "../includes/Core/Server.hpp"
#include "../includes/Core/ServerManager.hpp"
#include "../includes/Global/Logger.hpp"
#include "../includes/Global/MimeTypeResolver.hpp"
#include "../includes/Global/PerformanceMonitor.hpp"
#include "../includes/Global/StrUtils.hpp"

int main(int argc, char **argv)
{
	// Initialize logger session
	Logger::initializeSession("logs");

	// Initialize performance monitoring
	PerformanceMonitor &perfMonitor = PerformanceMonitor::getInstance();
	perfMonitor.setPerformanceThresholds(1000.0, 5000.0, 100 * 1024 * 1024); // 1s, 5s, 100MB
	Logger::log(Logger::INFO, "PerformanceMonitor: Performance monitoring initialized", __FILE__, __LINE__,
				__FUNCTION__);

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
		AST::ASTNode cfg = parser.parse();
		if (cfg.children.empty())
			throw std::runtime_error("No server blocks found in config file");
		parser.printAST(cfg); // Temporary for debugging

		// 2. Translate the AST into server objects
		ConfigTranslator translator(cfg);
		std::vector<Server> servers = translator.getServers();
		if (servers.empty())
			throw std::runtime_error("No valid server blocks found in config file");

		// 3. Build server map
		ServerMap serverMap(servers);
		// serverMap.printServerMap(); // Temporarily commented out to debug segfault

		// 4. Create manager instance with server map
		ServerManager serverManager(serverMap);

		// 5. Start the server
		serverManager.run();

		// Log final performance report
		perfMonitor.logPerformanceReport();
	}
	catch (const std::exception &e)
	{
		Logger::log(Logger::ERROR, "WebServ failed: " + std::string(e.what()), __FILE__, __LINE__, __PRETTY_FUNCTION__);

		// Log performance report even on failure
		perfMonitor.logPerformanceSummary();

		// Cleanup even on failure
		PerformanceMonitor::destroyInstance();
		MimeTypeResolver::cleanup();

		Logger::closeSession();
		return 1;
	}

	// Cleanup performance monitoring
	PerformanceMonitor::destroyInstance();

	// Cleanup MIME type resolver
	MimeTypeResolver::cleanup();

	Logger::closeSession();
	return 0;
}
