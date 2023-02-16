#include "level.h"

namespace stardust {
	Level::Level(const fs::path& lunar_magic_path, const fs::path& temporary_rom_path, const fs::path& mwl_file)
		: LunarMagicInsertable(lunar_magic_path, temporary_rom_path), mwl_file(mwl_file)
	{
		if (!fs::exists(mwl_file)) {
			throw ResourceNotFoundException(fmt::format(
				"Level file {} not found",
				mwl_file.string()
			));
		}
	}

	void Level::insert() {
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