#include "pixi.h"

namespace stardust {
	Pixi::Pixi(const fs::path& pixi_exe_path, const fs::path& temporary_rom_path, const std::string& pixi_options)
		: RomInsertable(temporary_rom_path), pixi_exe_path(pixi_exe_path), pixi_options(pixi_options)
	{
		if (!fs::exists(pixi_exe_path)) {
			throw ToolNotFoundException(fmt::format(
				"PIXI executable not found at {}",
				pixi_exe_path.string()
			));
		}
	}

	void Pixi::insert() {
		spdlog::info("Applying PIXI");
		spdlog::debug(fmt::format(
			"Applying PIXI using {} onto temporary ROM {}",
			pixi_exe_path.string(),
			temporary_rom_path.string()
		));

		const auto exit_code{ bp::system(fmt::format(
			"\"{}\" {} \"{}\"",
			pixi_exe_path.string(),
			pixi_options,
			temporary_rom_path.string()
		))};

		if (exit_code == 0) {
			spdlog::info("Successfully applied PIXI");
		}
		else {
			throw InsertionException(fmt::format(
				"Failed to apply PIXI onto temporary ROM {}",
				temporary_rom_path.string()
			));
		}
	}
}
