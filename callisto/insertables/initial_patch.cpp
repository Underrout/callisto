#include "initial_patch.h"

namespace callisto {
	InitialPatch::InitialPatch(const Configuration& config) 
		: RomInsertable(config),
		flips_path(registerConfigurationDependency(config.flips_path).getOrThrow()),
		clean_rom_path(registerConfigurationDependency(config.clean_rom).getOrThrow()),
		initial_patch_path(registerConfigurationDependency(config.initial_patch).getOrThrow())
	{
		if (!fs::exists(flips_path)) {
			throw ToolNotFoundException(fmt::format(
				colors::EXCEPTION,
				"FLIPS not found at {}",
				flips_path.string()
			));
		}

		if (!fs::exists(clean_rom_path)) {
			throw NotFoundException(fmt::format(
				colors::EXCEPTION,
				"Clean ROM not found at {}",
				clean_rom_path.string()
			));
		}

		if (!fs::exists(initial_patch_path)) {
			throw ResourceNotFoundException(fmt::format(
				colors::EXCEPTION,
				"Initial patch not found at {}",
				initial_patch_path.string()
			));
		}
	}

	std::unordered_set<ResourceDependency> InitialPatch::determineDependencies() {
		return {
			ResourceDependency(flips_path, Policy::REBUILD),
			ResourceDependency(initial_patch_path, Policy::REBUILD),
			ResourceDependency(clean_rom_path, Policy::REBUILD)
		};
	}

	void InitialPatch::insert() {
		insert(temporary_rom_path);
	}

	void InitialPatch::insert(const fs::path& target_rom) {
		spdlog::info(fmt::format(colors::RESOURCE, "Applying initial patch {}", initial_patch_path.string()));

		int exit_code{ bp::system(
			flips_path.string(),
			"--apply",
			initial_patch_path.string(),
			clean_rom_path.string(),
			target_rom.string()
		) };

		if (exit_code != 0) {
			throw InsertionException(fmt::format(
				colors::EXCEPTION,
				"Failed to apply initial patch {} to ROM {}",
				initial_patch_path.string(),
				target_rom.string()
			));
		}
		else {
			spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Successfully applied initial patch!"));
		}
	}
}
