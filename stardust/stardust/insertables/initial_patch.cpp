#include "initial_patch.h"

namespace stardust {
	InitialPatch::InitialPatch(const Configuration& config) 
		: RomInsertable(config),
		flips_path(registerConfigurationDependency(config.flips_path).getOrThrow()),
		clean_rom_path(registerConfigurationDependency(config.clean_rom).getOrThrow()),
		initial_patch_path(registerConfigurationDependency(config.initial_patch).getOrThrow())
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

	std::unordered_set<ResourceDependency> InitialPatch::determineDependencies() {
		return {
			ResourceDependency(flips_path),
			ResourceDependency(initial_patch_path),
			ResourceDependency(clean_rom_path)
		};
	}

	void InitialPatch::insert() {
		spdlog::info(fmt::format("Applying initial patch {}", initial_patch_path.string()));

		int exit_code{ bp::system(
			flips_path.string(),
			"--apply",
			initial_patch_path.string(),
			clean_rom_path.string(),
			temporary_rom_path.string()
		) };

		if (exit_code != 0) {
			throw InsertionException(fmt::format(
				"Failed to apply initial patch {} to temporary ROM {}",
				initial_patch_path.string(),
				temporary_rom_path.string()
			));
		}
		else {
			spdlog::info("Successfully applied initial patch!");
		}
	}
}
