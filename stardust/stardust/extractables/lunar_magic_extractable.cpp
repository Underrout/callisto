#include "lunar_magic_extractable.h"

namespace stardust {
	LunarMagicExtractable::LunarMagicExtractable(const Configuration& config)
		: lunar_magic_executable(config.lunar_magic_path.getOrThrow()), extracting_rom(config.project_rom.getOrThrow()){
		if (!fs::exists(lunar_magic_executable)) {
			throw ToolNotFoundException(fmt::format(
				"Lunar Magic not found at {}",
				lunar_magic_executable.string()
			));
		}

		if (!fs::exists(extracting_rom)) {
			throw ResourceNotFoundException(fmt::format(
				"Extracting ROM not found at {}",
				extracting_rom.string()
			));
		}
	}
}
