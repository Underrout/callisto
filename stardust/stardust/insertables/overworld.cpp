#include "overworld.h"

namespace stardust {
	const char* Overworld::OVERWORLD_ROM_POSTFIX = "_overworld";

	Overworld::Overworld(const fs::path& lunar_magic_path, const fs::path& flips_path,
			const fs::path& clean_rom_path, const fs::path& temporary_rom_path,
			const fs::path& overworld_bps_path)
		: FlipsInsertable(flips_path, clean_rom_path, lunar_magic_path, temporary_rom_path),
		overworld_bps_path(overworld_bps_path) {
		if (!fs::exists(overworld_bps_path)) {
			throw ResourceNotFoundException(fmt::format(
				"Overworld BPS patch not found at {}",
				overworld_bps_path.string()
			));
		}
	}

	fs::path Overworld::getTemporaryOverworldRomPath() const {
		return temporary_rom_path.stem().string()
			+ OVERWORLD_ROM_POSTFIX
			+ temporary_rom_path.extension().string();
	}

	fs::path Overworld::createTemporaryOverworldRom() const {
		spdlog::debug(fmt::format(
			"Creating temporary ROM of overworld from overworld BPS patch at {}",
			overworld_bps_path.string()
		));

		const auto rom_path{ getTemporaryOverworldRomPath() };
		const auto result{ bpsToRom(overworld_bps_path, rom_path) };

		if (result == 0) {
			spdlog::debug("Successfully created overworld ROM");
		}
		else {
			throw InsertionException(std::format(
				"Failed to create temporary overworld ROM at {} "
				"from overworld BPS patch at {} using the clean ROM from {}",
				rom_path.string(),
				overworld_bps_path.string(),
				clean_rom_path.string()
			));
		}

		return rom_path;
	}

	void Overworld::deleteTemporaryOverworldRom(const fs::path& temporary_ow_rom_path) const {
		try {
			fs::remove(temporary_ow_rom_path);
		}
		catch (const fs::filesystem_error& e) {
			spdlog::warn(fmt::format(
				"Failed to delete temporary Overworld ROM {}",
				temporary_ow_rom_path.string()
			));
		}
	}

	void Overworld::insert() {
		spdlog::info("Inserting Overworld");
		spdlog::debug(fmt::format(
			"Inserting Overworld from BPS patch {} into temporary ROM {}",
			overworld_bps_path.string(),
			temporary_rom_path.string()
		));

		const auto temporary_ow_rom_path{ createTemporaryOverworldRom() };
		const auto transfer_result{ callLunarMagic(
			"-TransferOverworld", 
			temporary_rom_path.string(), 
			temporary_ow_rom_path.string()
		)};

		if (transfer_result == 0) {
			spdlog::info("Successfully inserted Overworld!");
			spdlog::debug(fmt::format(
				"Successfully inserted Overworld from BPS patch {} into temporary ROM {}",
				overworld_bps_path.string(),
				temporary_rom_path.string()
			));
		}
		else {
			throw InsertionException(fmt::format(
				"Failed to insert Overworld from BPS patch {} into temporary ROM {}",
				overworld_bps_path.string(),
				temporary_rom_path.string()
			));
		}
	}
}
