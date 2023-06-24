#include "levels.h"

namespace callisto {
	namespace extractables {
		Levels::Levels(const Configuration& config, const fs::path& extracting_rom)
			: LunarMagicExtractable(config, extracting_rom), levels_folder(config.levels.getOrThrow()) {
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

		void Levels::normalize() const {
			spdlog::debug("Stripping source pointers from MWLs in directory {}", levels_folder.string());
			for (const auto& entry : fs::directory_iterator(levels_folder)) {
				if (entry.path().extension() == ".mwl") {
					Level::normalize(entry.path());
				}
			}
		}

		void Levels::extract() {
			spdlog::info("Exporting levels");

			fs::path temporary_levels_folder{ levels_folder.string() + "_temp" };
			fs::remove_all(temporary_levels_folder);
			fs::create_directories(temporary_levels_folder);

			spdlog::debug("Exporting levels to temporary folder {} from ROM {}", levels_folder.string(), extracting_rom.string());

			const auto exit_code{ callLunarMagic("-ExportMultLevels", 
				extracting_rom.string(), (temporary_levels_folder / "level").string()) };

			if (exit_code == 0) {
				spdlog::debug("Copying temporary folder {} to {}", temporary_levels_folder.string(), levels_folder.string());
				fs::remove_all(levels_folder);
				fs::copy(temporary_levels_folder, levels_folder);
				fs::remove_all(temporary_levels_folder);

				if (strip_source_pointers) {
					normalize();
				}
				spdlog::info("Successfully exported levels!");
			}
			else {
				fs::remove_all(temporary_levels_folder);
				throw ExtractionException(fmt::format(
					"Failed to export levels from ROM {} to directory {}",
					extracting_rom.string(), levels_folder.string()
				));
			}
		}
	}
}
