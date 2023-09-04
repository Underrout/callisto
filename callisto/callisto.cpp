#include "callisto.h"

using namespace callisto;

int main(int argc, char** argv) {
	if (argc == 1) {
		// run in interactive mode
		const auto plain_path{ fs::path(argv[0]) };
		const auto abs_path{ fs::canonical(plain_path) };
		TUI tui{ abs_path };
		tui.run();
	}
	else {
		// run in CLI mode
		CLIHandler::run(argc, argv);
	}
}
