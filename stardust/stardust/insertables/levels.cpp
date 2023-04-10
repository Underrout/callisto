#include "levels.h"

namespace stardust {
	Levels::Levels(const Configuration& config)
		: LunarMagicInsertable(config), 
		levels_folder(registerConfigurationDependency(config.levels, Policy::REINSERT).getOrThrow())
	{
		registerConfigurationDependency(config.level_import_flag);

		if (config.level_import_flag.isSet()) {
			import_flag = config.level_import_flag.getOrThrow();
		}
		else {
			import_flag = std::nullopt;
		}

		if (!fs::exists(levels_folder)) {
			throw ResourceNotFoundException(fmt::format(
				"Level folder {} not found",
				levels_folder.string()
			));
		}
	}

	std::unordered_set<ResourceDependency> Levels::determineDependencies() {
		return { ResourceDependency(levels_folder) };
	}

	void Levels::insert() {
		spdlog::info("Inserting levels");

		int exit_code;

		if (import_flag.has_value()) {
			exit_code = callLunarMagic(
				"-ImportMultLevels",
				temporary_rom_path.string(),
				levels_folder.string(),
				import_flag.value());
		}
		else {
			exit_code = callLunarMagic(
				"-ImportMultLevels",
				temporary_rom_path.string(),
				levels_folder.string());
		}

		if (exit_code == 0) {
			spdlog::info("Successfully inserted levels");
			spdlog::debug(fmt::format(
				"Successfully inserted levels from folder {} into temporary ROM {}",
				levels_folder.string(),
				temporary_rom_path.string()
			));
		}
		else {
			throw InsertionException(fmt::format(
				"Failed to insert level from folder {} into temporary ROM {}",
				levels_folder.string(),
				temporary_rom_path.string()
			));
		}
	}
}
