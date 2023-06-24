#include "lunar_magic_insertable.h"

namespace callisto {
	LunarMagicInsertable::LunarMagicInsertable(const Configuration& config)
		: RomInsertable(config), 
		lunar_magic_path(registerConfigurationDependency(config.lunar_magic_path, Policy::REINSERT).getOrThrow()) {
		if (!fs::exists(lunar_magic_path)) {
			throw ToolNotFoundException(fmt::format(
				"Lunar Magic not found at {}",
				lunar_magic_path.string()
			));
		}
	}

	std::unordered_set<ResourceDependency> LunarMagicInsertable::determineDependencies() {
		return { ResourceDependency(lunar_magic_path, Policy::REBUILD) };
	}
}
