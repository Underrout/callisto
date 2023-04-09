#include "level.h"

namespace stardust {
	Level::Level(const Configuration& config, const fs::path& mwl_file)
		: LunarMagicInsertable(config), mwl_file(mwl_file)
	{
		if (!fs::exists(mwl_file)) {
			throw ResourceNotFoundException(fmt::format(
				"Level file {} not found",
				mwl_file.string()
			));
		}
	}

	std::unordered_set<ResourceDependency> Level::determineDependencies() {
		auto dependencies{ LunarMagicInsertable::determineDependencies() };
		dependencies.insert(ResourceDependency(mwl_file));
		return dependencies;
	}

	void Level::insert() {
		spdlog::info("Inserting {}", mwl_file.filename().string());
		spdlog::debug(fmt::format(
			"Inserting level from MWL file {} into temporary ROM {}",
			mwl_file.string(),
			temporary_rom_path.string()
		));

		const auto exit_code{ callLunarMagic(
			"-ImportLevel", 
			temporary_rom_path.string(), 
			mwl_file.string()) 
		};

		if (exit_code == 0) {
			spdlog::info("Successfully inserted {}", mwl_file.filename().string());
			spdlog::debug(fmt::format(
				"Successfully inserted level from MWL file {} into temporary ROM {}",
				mwl_file.string(),
				temporary_rom_path.string()
			));
		}
		else {
			throw InsertionException(fmt::format(
				"Failed to insert level from file {} into temporary ROM {}",
				mwl_file.string(),
				temporary_rom_path.string()
			));
		}
	}
}
