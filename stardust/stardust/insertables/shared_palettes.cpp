#include "shared_palettes.h"

namespace stardust {
	SharedPalettes::SharedPalettes(const fs::path& lunar_magic_path, const fs::path& temporary_rom_path, const fs::path& shared_palettes_path)
		: LunarMagicInsertable(lunar_magic_path, temporary_rom_path), shared_palettes_path(shared_palettes_path)
	{
		if (!fs::exists(shared_palettes_path)) {
			throw ResourceNotFoundException(fmt::format(
				"Shared Palette file not found at {}",
				shared_palettes_path.string()
			));
		}
	}

	void SharedPalettes::insert() {
		spdlog::info("Inserting Shared Palettes");
		spdlog::debug(fmt::format(
			"Inserting Shared Palette file {} into temporary ROM {}",
			shared_palettes_path.string(),
			temporary_rom_path.string()
		));

		const auto exit_code{ callLunarMagic("-ImportSharedPalette", 
			temporary_rom_path.string(), shared_palettes_path.string()) };

		if (exit_code == 0) {
			spdlog::info("Successfully inserted Shared Palettes!");
			spdlog::debug(fmt::format(
				"Successfully inserted Shared Palette file {} into temporary ROM {}",
				shared_palettes_path.string(),
				temporary_rom_path.string()
			));
		}
		else {
			throw InsertionException(fmt::format(
				"Failed to insert Shared Palette file {} into temporary ROM {}",
				shared_palettes_path.string(),
				temporary_rom_path.string()
			));
		}
	}
}
