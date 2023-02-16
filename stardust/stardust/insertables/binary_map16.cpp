#include "binary_map16.h"

namespace stardust {
	BinaryMap16::BinaryMap16(const fs::path& lunar_magic_path, const fs::path& temporary_rom_path, const fs::path& map16_file_path)
		: LunarMagicInsertable(lunar_magic_path, temporary_rom_path), map16_file_path(map16_file_path)
	{
		if (!fs::exists(map16_file_path)) {
			throw ResourceNotFoundException(fmt::format(
				"Binary Map16 file not found at {}",
				map16_file_path.string()
			));
		}
	}

	void BinaryMap16::insert() {
		spdlog::info("Inserting Map16");
		spdlog::debug(fmt::format(
			"Inserting binary map16 file at {} into temporary ROM at {}",
			map16_file_path.string(),
			temporary_rom_path.string()
		));

		const auto exit_code{ callLunarMagic(
			"-ImportAllMap16", 
			temporary_rom_path.string(), 
			map16_file_path.string())
		};

		if (exit_code == 0) {
			spdlog::info("Successfully inserted Map16!");
		}
		else {
			throw InsertionException(fmt::format(
				"Failed to insert binary Map16 file {} into temporary ROM {}",
				map16_file_path.string(),
				temporary_rom_path.string()
			));
		}
	}
}
