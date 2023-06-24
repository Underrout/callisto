#include <filesystem>

#include "tui/tui.h"

namespace fs = std::filesystem;

using namespace callisto;

int main(int argc, const char* argv[]) {
	TUI tui{ fs::path(std::string(argv[0])) };
	tui.run();
}
