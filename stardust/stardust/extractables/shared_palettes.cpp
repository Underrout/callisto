#include "shared_palettes.h"

namespace stardust {
	namespace extractables {
		SharedPalettes::SharedPalettes(const Configuration& config)
			: LunarMagicExtractable(config),
			shared_palettes_path(config.shared_palettes.getOrThrow()) {}

		void SharedPalettes::extract() {
			spdlog::info("Exporting Shared Palettes");
			spdlog::debug(fmt::format(
				"Exporting Shared Palette file {} from ROM {}",
				shared_palettes_path.string(),
				extracting_rom.string()
			));

			const auto exit_code{ callLunarMagic("-ExportSharedPalette",
				extracting_rom.string(), shared_palettes_path.string()) };

			if (exit_code == 0) {
				spdlog::info("Successfully exported Shared Palettes!");
				spdlog::debug(fmt::format(
					"Successfully exported Shared Palette file {} from ROM {}",
					shared_palettes_path.string(),
					extracting_rom.string()
				));
			}
			else {
				throw ExtractionException(fmt::format(
					"Failed to export Shared Palette file {} from ROM {}",
					shared_palettes_path.string(),
					extracting_rom.string()
				));
			}
		}
	}
}
