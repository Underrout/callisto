#include "text_map16.h"

namespace callisto {
	namespace extractables {
		TextMap16::TextMap16(const Configuration& config, const fs::path& extracting_rom)
			: LunarMagicExtractable(config, extracting_rom), map16_folder_path(config.map16.getOrThrow()) {}

		fs::path TextMap16::getTemporaryMap16FilePath() const {
			return extracting_rom.parent_path() / (map16_folder_path.string() + ".map16");
		}

		void TextMap16::exportTemporaryMap16File() const {
			const auto temp_map16{ getTemporaryMap16FilePath() };

			spdlog::debug(
				"Exporting temporary map16 file {} from ROM {}",
				temp_map16.string(),
				extracting_rom.string()
			);

			const auto exit_code{ callLunarMagic(
				"-ExportAllMap16",
				extracting_rom.string(),
				temp_map16.string())
			};

			if (exit_code == 0) {
				spdlog::debug(
					"Successfully exported temporary map16 file {} from ROM {}",
					temp_map16.string(),
					extracting_rom.string()
				);
			}
			else {
				throw ExtractionException(fmt::format(colors::EXCEPTION, "Failed to export temporary map16 file to {} from ROM {}",
					temp_map16.string(), extracting_rom.string()));
			}
		}

		void TextMap16::deleteTemporaryMap16File() const
		{
			try {
				fs::remove(getTemporaryMap16FilePath());
			}
			catch (const fs::filesystem_error& e) {
				spdlog::warn(fmt::format(
					colors::WARNING,
					"WARNING: Failed to delete temporary Map16 file {} with exception:\n\r{}",
					getTemporaryMap16FilePath().string(),
					e.what()
				));
			}
		}

		void TextMap16::extract() {
			spdlog::info(fmt::format(colors::RESOURCE, "Exporting Map16 folder"));
			spdlog::debug(
				"Exporting map16 folder {} from ROM {}",
				map16_folder_path.string(),
				extracting_rom.string()
			);

			exportTemporaryMap16File();
			const auto current_path{ fs::current_path() };

			try {
				HumanReadableMap16::from_map16::convert(getTemporaryMap16FilePath(), map16_folder_path);
			}
			catch (HumanMap16Exception& e) {
				fs::current_path(current_path);
				deleteTemporaryMap16File();
				throw ExtractionException(fmt::format(colors::EXCEPTION, "Failed to convert map16 folder to file with exception:\n\r{}",
					e.get_detailed_error_message()));
			}

			fs::current_path(current_path);
			spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Successfully exported Map16 folder!"));

			deleteTemporaryMap16File();
		}
	}
}
