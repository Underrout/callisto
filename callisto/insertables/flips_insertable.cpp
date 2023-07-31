#include "flips_insertable.h"

namespace callisto {
	FlipsInsertable::FlipsInsertable(const Configuration& config, const PathConfigVariable& bps_patch_path)
		: LunarMagicInsertable(config), 
		flips_path(registerConfigurationDependency(config.flips_path).getOrThrow()), 
		clean_rom_path(registerConfigurationDependency(config.clean_rom).getOrThrow()), 
		bps_patch_path(registerConfigurationDependency(bps_patch_path).getOrThrow()),
		config(config)
	{
		for (const auto& descriptor : config.build_order) {
			if (descriptor.symbol == Symbol::GRAPHICS) {
				needs_gfx = true;
			}
			if (descriptor.symbol == Symbol::EX_GRAPHICS) {
				needs_exgfx = true;
			}
		}

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

	std::unordered_set<ResourceDependency> FlipsInsertable::determineDependencies() {
		auto dependencies{ LunarMagicInsertable::determineDependencies() };
		dependencies.insert(ResourceDependency(flips_path, Policy::REBUILD));
		dependencies.insert(ResourceDependency(bps_patch_path));
		dependencies.insert(ResourceDependency(clean_rom_path, Policy::REBUILD));
		return dependencies;
	}

	void FlipsInsertable::checkPatchExists() const {
		if (!fs::exists(bps_patch_path)) {
			throw ResourceNotFoundException(fmt::format(
				"{} BPS patch not found at {}",
				getResourceName(),
				bps_patch_path.string()
			));
		}
	}

	int FlipsInsertable::bpsToRom(const fs::path& bps_path, const fs::path& output_rom_path) const {
		spdlog::debug(fmt::format(
			"Creating ROM {} from {} with clean ROM {}",
			output_rom_path.string(),
			bps_path.string(),
			clean_rom_path.string()
		));
		int exit_code{ bp::system(flips_path.string(), "--apply", bps_path.string(),
			clean_rom_path.string(), output_rom_path.string()) };

		if (exit_code == 0) {
			spdlog::debug(fmt::format("Successfully patched to {}", output_rom_path.string()));
		}
		else {
			spdlog::debug(fmt::format(
				"Failed to create ROM {} from BPS patch {} with exit code {}",
				output_rom_path.string(), 
				bps_path.string(),
				exit_code
			));
		}

		return exit_code;
	}

	fs::path FlipsInsertable::getTemporaryPatchedRomPath() const {
		return temporary_rom_path.parent_path() / (temporary_rom_path.stem().string()
			+ getTemporaryPatchedRomPostfix()
			+ temporary_rom_path.extension().string());
	}

	fs::path FlipsInsertable::createTemporaryPatchedRom() const {
		spdlog::debug(fmt::format(
			"Creating temporary {} ROM from BPS patch at {}",
			getResourceName(),
			bps_patch_path.string()
		));

		const auto rom_path{ getTemporaryPatchedRomPath() };
		const auto result{ bpsToRom(bps_patch_path, rom_path) };

		if (result == 0) {
			spdlog::debug("Successfully created overworld ROM");
		}
		else {
			throw InsertionException(fmt::format(
				"Failed to create temporary {} ROM at {} "
				"from BPS patch at {} using the clean ROM from {}",
				getResourceName(),
				rom_path.string(),
				bps_patch_path.string(),
				clean_rom_path.string()
			));
		}

		return rom_path;
	}

	void FlipsInsertable::deleteTemporaryPatchedRom(const fs::path& temporary_patch_path) const {
		try {
			fs::remove(temporary_patch_path);
		}
		catch (const fs::filesystem_error&) {
			spdlog::warn(fmt::format(
				"Failed to delete temporary {} ROM {}",
				getResourceName(),
				temporary_patch_path.string()
			));
		}
	}

	void FlipsInsertable::init() {
		temporary_patched_rom_path = createTemporaryPatchedRom();

		if (needs_gfx) {
			GraphicsUtil::importProjectGraphicsInto(config, temporary_patched_rom_path);
		}

		if (needs_exgfx) {
			GraphicsUtil::importProjectExGraphicsInto(config, temporary_patched_rom_path);
		}
	}

	void FlipsInsertable::insert() {
		spdlog::info(fmt::format("Inserting {}", getResourceName()));
		spdlog::debug(fmt::format(
			"Inserting {} from BPS patch {} into temporary ROM {}",
			getResourceName(),
			bps_patch_path.string(),
			temporary_rom_path.string()
		));

		const auto transfer_result{ callLunarMagic(
			getLunarMagicFlag(),
			temporary_rom_path.string(),
			temporary_patched_rom_path.string()
		) };

		deleteTemporaryPatchedRom(temporary_patched_rom_path);

		if (transfer_result == 0) {
			spdlog::info(fmt::format("Successfully inserted {}!", getResourceName()));
			spdlog::debug(fmt::format(
				"Successfully transferred {} from {} to temporary ROM {}",
				getResourceName(),
				temporary_patched_rom_path.string(),
				temporary_rom_path.string()
			));
		}
		else {
			throw InsertionException(fmt::format(
				"Failed to transfer {} from {} to temporary ROM {}",
				getResourceName(),
				temporary_patched_rom_path.string(),
				temporary_rom_path.string()
			));
		}
	}
}
