#include "flips_extractable.h"

namespace stardust {
	FlipsExtractable::FlipsExtractable(const Configuration& config, const fs::path& output_patch_path, const fs::path& extracting_rom)
		: LunarMagicExtractable(config, extracting_rom), flips_executable(config.flips_path.getOrThrow()),
		clean_rom_path(config.clean_rom.getOrThrow()),
		output_patch_path(output_patch_path) {

		if (!fs::exists(flips_executable)) {
			throw ToolNotFoundException(fmt::format(
				"FLIPS not found at {}",
				flips_executable.string()
			));
		}

		if (!fs::exists(clean_rom_path)) {
			throw NotFoundException(fmt::format(
				"Clean ROM not found at {}",
				clean_rom_path.string()
			));
		}
	}

	void FlipsExtractable::extract() {
		const auto temp_rom{ getTemporaryResourceRomPath() };
		spdlog::info("Exporting {}", getResourceName());
		createTemporaryResourceRom(temp_rom);
		invokeLunarMagic(temp_rom);
		createOutputPatch(temp_rom);
		deleteTemporaryResourceRom(temp_rom);
	}

	fs::path FlipsExtractable::getTemporaryResourceRomPath() const {
		return extracting_rom.parent_path() / (extracting_rom.stem().string()
			+ getTemporaryResourceRomPostfix()
			+ extracting_rom.extension().string());
	}

	void FlipsExtractable::createTemporaryResourceRom(const fs::path& temporary_resource_rom) const {
		spdlog::debug("Copying clean ROM from {} to {}", clean_rom_path.string(), temporary_resource_rom.string());
		try {
			fs::copy_file(clean_rom_path, temporary_resource_rom);
		}
		catch (const fs::filesystem_error& e) {
			throw ExtractionException(fmt::format(
				"Failed to copy clean ROM from {} to {}",
				clean_rom_path.string(), temporary_resource_rom.string()
			));
		}
	}

	void FlipsExtractable::invokeLunarMagic(const fs::path& temporary_resource_rom) const {
		spdlog::debug("Invoking Lunar Magic at {}", lunar_magic_executable.string());
		const auto exit_code{ callLunarMagic(getLunarMagicFlag(), 
			temporary_resource_rom.string(), extracting_rom.string()) };

		if (exit_code == 0) {
			spdlog::debug("Successfully transferred {} from {} to {}", 
				getResourceName(), extracting_rom.string(), temporary_resource_rom.string());
		}
		else {
			throw ExtractionException(fmt::format(
				"Failed to transfer {} from {} to {}",
				getResourceName(), extracting_rom.string(), temporary_resource_rom.string()
			));
		}
	}

	void FlipsExtractable::createOutputPatch(const fs::path& temporary_resource_rom) const {
		spdlog::debug("Creating output patch {} from temporary ROM {}",
			output_patch_path.string(), temporary_resource_rom.string());

		const auto exit_code{ bp::system(
			flips_executable.string(), "--create", "--bps-delta", clean_rom_path.string(), 
			temporary_resource_rom.string(), output_patch_path.string()
		) };

		if (exit_code == 0) {
			spdlog::debug("Successfully created patch {} from temporary ROM {}",
				output_patch_path.string(),  temporary_resource_rom.string());
			spdlog::info("Successfully exported {}!", getResourceName());
		}
		else {
			throw ExtractionException(fmt::format(
				"Failed to create BPS patch {} from temporary ROM {}",
				output_patch_path.string(), temporary_resource_rom.string()
			));
		}
	}

	void FlipsExtractable::deleteTemporaryResourceRom(const fs::path& temporary_resource_rom) const {
		try {
			fs::remove(temporary_resource_rom);
		}
		catch (const fs::filesystem_error& e) {
			spdlog::warn("Failed to remove temporary ROM {}", temporary_resource_rom.string());
		}
	}
}
