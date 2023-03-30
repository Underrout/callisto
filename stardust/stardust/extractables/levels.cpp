#include "levels.h"

namespace stardust {
	namespace extractables {
		Levels::Levels(const Configuration& config)
			: LunarMagicExtractable(config), levels_folder(config.levels.getOrThrow()) {
			if (!fs::exists(levels_folder)) {
				spdlog::debug("Levels folder {} does not existing, creating it now", levels_folder.string());
				try {
					fs::create_directories(levels_folder);
				}
				catch (const fs::filesystem_error&) {
					throw ExtractionException(fmt::format("Failed to create levels directory {}", levels_folder.string()));
				}
			}
		}

		void Levels::stripSourcePointers() const {
			spdlog::debug("Stripping source pointers from MWLs in directory {}", levels_folder.string());
			for (const auto& entry : fs::directory_iterator(levels_folder)) {
				if (entry.path().extension() == ".mwl") {
					Level::stripSourcePointers(entry.path());
				}
			}
		}

		void Levels::extract() {
			spdlog::info("Exporting levels");
			spdlog::debug("Exporting levels to {} from ROM {}", levels_folder.string(), extracting_rom.string());

			const auto exit_code{ callLunarMagic("-ExportMultLevels", 
				extracting_rom.string(), (levels_folder / "level").string()) };

			if (exit_code == 0) {
				if (strip_source_pointers) {
					stripSourcePointers();
				}
				spdlog::info("Successfully exported levels!");
			}
			else {
				throw ExtractionException(fmt::format(
					"Failed to export levels from ROM {} to directory {}",
					extracting_rom.string(), levels_folder.string()
				));
			}
		}
	}
}
