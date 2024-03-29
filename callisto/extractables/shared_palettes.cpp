#include "shared_palettes.h"

namespace callisto {
	namespace extractables {
		SharedPalettes::SharedPalettes(const Configuration& config, const fs::path& extracting_rom)
			: LunarMagicExtractable(config, extracting_rom),
			shared_palettes_path(config.shared_palettes.getOrThrow()) {}

		void SharedPalettes::extract() {
			spdlog::info(fmt::format(colors::RESOURCE, "Exporting Shared Palettes"));
			spdlog::debug(fmt::format(
				"Exporting Shared Palette file {} from ROM {}",
				shared_palettes_path.string(),
				extracting_rom.string()
			));

			const auto exit_code{ callLunarMagic("-ExportSharedPalette",
				extracting_rom.string(), shared_palettes_path.string()) };

			if (exit_code == 0) {
				spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Successfully exported Shared Palettes!"));
				spdlog::debug(fmt::format(
					"Successfully exported Shared Palette file {} from ROM {}",
					shared_palettes_path.string(),
					extracting_rom.string()
				));
			}
			else {
				throw ExtractionException(fmt::format(
					colors::EXCEPTION,
					"Failed to export Shared Palette file {} from ROM {}",
					shared_palettes_path.string(),
					extracting_rom.string()
				));
			}
		}
	}
}
