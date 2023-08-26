#pragma once

#include <optional>
#include <filesystem>

#include <CLI/CLI.hpp>
#include <fmt/format.h>

#include "../configuration/configuration_manager.h"
#include "../builders/rebuilder.h"
#include "../builders/quick_builder.h"
#include "../saver/saver.h"
#include "../saver/marker.h"

#include "../globals.h"

#include "../lunar_magic/lunar_magic_wrapper.h"

namespace fs = std::filesystem;

namespace callisto {
	class CLIHandler {
	public:
		static int run(int argc, char** argv);
	};
}
