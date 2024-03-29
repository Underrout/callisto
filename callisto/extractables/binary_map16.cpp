#include "binary_map16.h"

namespace callisto {
	namespace extractables {
		BinaryMap16::BinaryMap16(const Configuration& config, const fs::path& extracting_rom)
			: LunarMagicExtractable(config, extracting_rom), map16_file_path(config.map16.getOrThrow()) {}

		void BinaryMap16::extract() {
			spdlog::info(fmt::format(colors::RESOURCE, "Exporting Map16"));
			spdlog::debug(
				"Exporting binary map16 file {} from ROM at {}",
				map16_file_path.string(),
				extracting_rom.string()
			);

			const auto exit_code{ callLunarMagic(
				"-ExportAllMap16",
				extracting_rom.string(),
				map16_file_path.string())
			};

			if (exit_code == 0) {
				spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Successfully exported Map16!"));
			}
			else {
				throw ExtractionException(fmt::format(
					colors::EXCEPTION,
					"Failed to export binary Map16 file {} from ROM {}",
					map16_file_path.string(),
					extracting_rom.string()
				));
			}
		}
	}
}
