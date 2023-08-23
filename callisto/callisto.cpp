#include "callisto.h"

using namespace callisto;

int main(int argc, char** argv) {
	if (argc == 1) {
		// run in interactive mode
		TUI tui{ fs::path(std::string(argv[0])) };
		tui.run();
	}
	else {
		// run in CLI mode
		CLIHandler::run(argc, argv);
	}
}
