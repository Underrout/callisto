#pragma once

#include <filesystem>

#include <toml.hpp>
#include <fmt/core.h>

#include "config_variable.h"

namespace fs = std::filesystem;

namespace stardust {
	class Configuration {
	public:
		StringConfigVariable config_name{ {"settings", "config_name"} };
		PathConfigVariable project_root{ {"settings", "project_root"} };
		IntegerConfigVariable rom_size{ {"settings", "rom_size"} };

		PathConfigVariable project_rom{ {"output", "project_rom"} };
		PathConfigVariable temporary_rom{ {"output", "temporary_rom"} };
		PathConfigVariable bps_package{ {"output", "bps_package"} };
	};
}
