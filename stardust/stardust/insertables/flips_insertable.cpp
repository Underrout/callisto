#include "flips_insertable.h"

namespace stardust {
	FlipsInsertable::FlipsInsertable(const fs::path& flips_path, const fs::path& clean_rom_path, 
		const fs::path& lunar_magic_path, const fs::path& temporary_rom_path) 
		: LunarMagicInsertable(lunar_magic_path, temporary_rom_path), flips_path(flips_path), 
		clean_rom_path(clean_rom_path)
	{
		if (!fs::exists(flips_path)) {
			throw ToolNotFoundException(fmt::format(
				"FLIPS not found at {}",
				flips_path.string()
			));
		} 

		if (!fs::exists(clean_rom_path)) {
			throw NotFoundException(fmt::format(
				"Clean ROM not found at {}",
				clean_rom_path.string()
			));
		}
	}

	int FlipsInsertable::bpsToRom(const fs::path& bps_path, const fs::path& output_rom_path) {
		spdlog::debug(fmt::format(
			"Creating ROM {} from {} with clean ROM {}",
			output_rom_path.string(),
			bps_path.string(),
			clean_rom_path.string()
		));
		return bp::system(flips_path.string(), "--apply", bps_path.string(),
			clean_rom_path.string(), output_rom_path.string());
	}
}
