#pragma once

#include <filesystem>
#include <map>

#include <toml.hpp>
#include <fmt/core.h>

#include "config_variable.h"
#include "config_exception.h"

namespace fs = std::filesystem;

namespace stardust {
	class Configuration {
	private:

	public:
		static std::map<std::string, std::string> parseUserVariables(const std::vector<toml::value>& tables,
			const std::map<std::string, std::string>& previous_user_variables = {});
		StringConfigVariable config_name{ {"settings", "config_name"} };
		PathConfigVariable project_root{ {"settings", "project_root"} };
		IntegerConfigVariable rom_size{ {"settings", "rom_size"} };

		PathConfigVariable project_rom{ {"output", "project_rom"} };
		PathConfigVariable temporary_rom{ {"output", "temporary_rom"} };
		PathConfigVariable bps_package{ {"output", "bps_package"} };

		PathConfigVariable flips_path{ {"flips", "executable"} };

		PathConfigVariable lunar_magic_path{ {"lunar_magic", "executable"} };
		StringConfigVariable lunar_magic_level_import_flags{ {"LunarMagic", "level_import_flags"} };

		PathConfigVariable initial_patch{ {"resources", "initial_patch"} };
		PathConfigVariable levels{ {"resources", "levels"} };
		PathConfigVariable shared_palettes{ {"resources", "shared_palettes"} };
		PathConfigVariable map16{ {"resources", "map16"} };
		PathConfigVariable overworld{ {"resources", "overworld"} };
		PathConfigVariable titlescreen{ {"resources", "titlescreen"} };
		PathConfigVariable credits{ {"resources", "credits"} };
		PathConfigVariable global_exanimation{ {"resources", "global_exanimation"} };

		ExtendablePathVectorConfigVariable patches{ {"resources", "patches"} };
		ExtendablePathVectorConfigVariable globules{ {"resources", "globules"} };

		PathConfigVariable globule_header{ {"resources", "globule_header"} };

		StringVectorConfigVariable build_order{ {"orders", "build_order"} };
	};
}
