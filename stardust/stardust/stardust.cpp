#include <filesystem>

#include "tui/tui.h"

namespace fs = std::filesystem;

using namespace stardust;

int main(int argc, const char* argv[]) {
	TUI tui{ fs::path(std::string(argv[0])) };
	tui.run();
}
