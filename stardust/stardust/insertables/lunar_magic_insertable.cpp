#include "lunar_magic_insertable.h"

namespace stardust {
	LunarMagicInsertable::LunarMagicInsertable(const fs::path& lunar_magic_path, const fs::path& temporary_rom_path)
		: RomInsertable(temporary_rom_path), lunar_magic_path(lunar_magic_path) {
		if (!fs::exists(lunar_magic_path)) {
			throw ToolNotFoundException(fmt::format(
				"Lunar Magic not found at {}",
				lunar_magic_path.string()
			));
		}
	}

	std::unordered_set<Dependency> LunarMagicInsertable::determineDependencies() {
		return { Dependency(lunar_magic_path) };
	}
}
