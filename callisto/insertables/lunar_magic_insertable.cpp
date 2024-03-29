#include "lunar_magic_insertable.h"

namespace callisto {
	LunarMagicInsertable::LunarMagicInsertable(const Configuration& config)
		: RomInsertable(config), 
		lunar_magic_path(registerConfigurationDependency(config.lunar_magic_path, Policy::REINSERT).getOrThrow()) {
	}

	std::unordered_set<ResourceDependency> LunarMagicInsertable::determineDependencies() {
		return { ResourceDependency(lunar_magic_path, Policy::REBUILD) };
	}

	void LunarMagicInsertable::checkLunarMagicExists() {
		if (!fs::exists(lunar_magic_path)) {
			throw ToolNotFoundException(fmt::format(
				colors::EXCEPTION,
				"Lunar Magic not found at {}",
				lunar_magic_path.string()
			));
		}
	}
}
