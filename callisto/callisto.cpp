#include "callisto.h"

using namespace callisto;

int main(int argc, char** argv) {
	if (argc == 1) {
		// run in interactive mode
		TUI tui{ fs::path(std::string(argv[0])) };
		tui.run();
	}
	else if (argc == 6 && std::string(argv[1]) == "magic") {
		// pass along info from Lunar Magic to callisto
		spdlog::set_level(spdlog::level::debug);
		spdlog::set_pattern("[%^%l%$] %v");
		LunarMagicWrapper::communicate(argv[3], argv[2], argv[4], argv[5]);
	}
	else {
		// run in CLI mode
		CLIHandler::run(argc, argv);
	}
}
