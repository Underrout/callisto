#include "shared_palettes.h"

namespace callisto {
	SharedPalettes::SharedPalettes(const Configuration& config)
		: LunarMagicInsertable(config), 
		shared_palettes_path(registerConfigurationDependency(config.shared_palettes, Policy::REINSERT).getOrThrow())
	{

	}

	std::unordered_set<ResourceDependency> SharedPalettes::determineDependencies() {
		auto dependencies{ LunarMagicInsertable::determineDependencies() };
		dependencies.insert(shared_palettes_path);
		return dependencies;
	}

	void SharedPalettes::insert() {
		if (!fs::exists(shared_palettes_path)) {
			throw ResourceNotFoundException(fmt::format(
				colors::EXCEPTION,
				"Shared Palette file not found at {}",
				shared_palettes_path.string()
			));
		}

		spdlog::info(fmt::format(colors::RESOURCE, "Inserting Shared Palettes"));
		spdlog::debug(fmt::format(
			"Inserting Shared Palette file {} into temporary ROM {}",
			shared_palettes_path.string(),
			temporary_rom_path.string()
		));

		const auto exit_code{ callLunarMagic("-ImportSharedPalette", 
			temporary_rom_path.string(), shared_palettes_path.string()) };

		if (exit_code == 0) {
			spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Successfully inserted Shared Palettes!"));
			spdlog::debug(fmt::format(
				"Successfully inserted Shared Palette file {} into temporary ROM {}",
				shared_palettes_path.string(),
				temporary_rom_path.string()
			));
		}
		else {
			throw InsertionException(fmt::format(
				colors::EXCEPTION,
				"Failed to insert Shared Palette file {} into temporary ROM {}",
				shared_palettes_path.string(),
				temporary_rom_path.string()
			));
		}
	}
}
