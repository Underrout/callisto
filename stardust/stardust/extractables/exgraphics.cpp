#include "exgraphics.h"

namespace stardust {
	namespace extractables {
		ExGraphics::ExGraphics(const Configuration& config, const fs::path& extracting_rom)
			: LunarMagicExtractable(config, extracting_rom),
			source_exgraphics_folder(extracting_rom.parent_path() / EXGRAPHICS_FOLDER_NAME),
			target_exgraphics_folder(config.project_rom.getOrThrow().parent_path() / EXGRAPHICS_FOLDER_NAME) {}

		void ExGraphics::extract() {
			spdlog::info("Exporting ExGraphics");
			spdlog::debug(fmt::format(
				"Exporting ExGraphics from ROM {} to folder {}",
				extracting_rom.string(),
				source_exgraphics_folder.string()
			));

			const auto exit_code{ callLunarMagic("-ExportExGFX", extracting_rom.string()) };

			if (exit_code == 0) {
				spdlog::debug("Successfully exported ExGFX to {}", source_exgraphics_folder.string());

				if (source_exgraphics_folder != target_exgraphics_folder) {
					spdlog::debug("Moving {} to final output ExGFX folder {}", source_exgraphics_folder.string(), target_exgraphics_folder.string());
					try {
						fs::copy(source_exgraphics_folder.string(), target_exgraphics_folder.string());
					}
					catch (const std::exception& e) {
						throw ExtractionException(fmt::format(
							"Failed to copy {} to {} with exception:\n{}",
							source_exgraphics_folder.string(), target_exgraphics_folder.string(),
							e.what()
						));
					}
					try {
						fs::remove_all(source_exgraphics_folder);
					}
					catch (const std::exception& e) {
						spdlog::warn("Failed to remove intermediate ExGFX folder {}", source_exgraphics_folder.string());
					}
				}

				spdlog::info("Successfully exported ExGraphics!");
				spdlog::debug(fmt::format(
					"Successfully exported ExGraphics from ROM {} to {}",
					extracting_rom.string(),
					target_exgraphics_folder.string()
				));
			}
			else {
				throw ExtractionException(fmt::format(
					"Failed to export ExGraphics from ROM {} to {}",
					extracting_rom.string(),
					source_exgraphics_folder.string()
				));
			}
		}
	}
}
