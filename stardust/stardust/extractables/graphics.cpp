#include "graphics.h"

namespace stardust {
	namespace extractables {
		Graphics::Graphics(const Configuration& config, const fs::path& extracting_rom)
			: LunarMagicExtractable(config, extracting_rom),
			source_graphics_folder(extracting_rom.parent_path() / GRAPHICS_FOLDER_NAME),
			target_graphics_folder(config.project_rom.getOrThrow().parent_path() / GRAPHICS_FOLDER_NAME) {}

		void Graphics::extract() {
			spdlog::info("Exporting Graphics");
			spdlog::debug(fmt::format(
				"Exporting Graphics from ROM {} to folder {}",
				extracting_rom.string(),
				source_graphics_folder.string()
			));

			const auto exit_code{ callLunarMagic("-ExportGFX", extracting_rom.string()) };

			if (exit_code == 0) {
				spdlog::debug("Successfully exported GFX to {}", source_graphics_folder.string());

				if (source_graphics_folder != target_graphics_folder) {
					spdlog::debug("Moving {} to final output GFX folder {}", source_graphics_folder.string(), target_graphics_folder.string());
					try {
						fs::copy(source_graphics_folder.string(), target_graphics_folder.string());
					}
					catch (const std::exception& e) {
						throw ExtractionException(fmt::format(
							"Failed to copy {} to {} with exception:\n{}",
							source_graphics_folder.string(), target_graphics_folder.string(),
							e.what()
						));
					}
					try {
						fs::remove_all(source_graphics_folder);
					}
					catch (const std::exception& e) {
						spdlog::warn("Failed to remove intermediate ExGFX folder {}", source_graphics_folder.string());
					}
				}

				spdlog::info("Successfully exported Graphics!");
				spdlog::debug(fmt::format(
					"Successfully exported Graphics from ROM {} to {}",
					extracting_rom.string(),
					target_graphics_folder.string()
				));
			}
			else {
				throw ExtractionException(fmt::format(
					"Failed to export Graphics from ROM {} to {}",
					extracting_rom.string(),
					source_graphics_folder.string()
				));
			}
		}
	}
}
