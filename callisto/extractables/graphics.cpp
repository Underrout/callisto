#include "graphics.h"

namespace callisto {
	namespace extractables {
		Graphics::Graphics(const Configuration& config, const fs::path& extracting_rom)
			: LunarMagicExtractable(config, extracting_rom), config(config) {}

		void Graphics::extract() {
			spdlog::info(fmt::format(colors::RESOURCE, "Exporting Graphics"));

			const auto keep_symlink{ extracting_rom == config.output_rom.getOrThrow() };

			try {
				GraphicsUtil::exportProjectGraphicsFrom(config, extracting_rom, keep_symlink);
			}
			catch (const std::exception& e) {
				throw ExtractionException(fmt::format(
					colors::EXCEPTION,
					"Failed to export Graphics from ROM {} with exception:\n\r{}",
					extracting_rom.string(), e.what()
				));
			}
		}
	}
}
