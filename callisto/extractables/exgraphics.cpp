#include "exgraphics.h"

namespace callisto {
	namespace extractables {
		ExGraphics::ExGraphics(const Configuration& config, const fs::path& extracting_rom)
			: LunarMagicExtractable(config, extracting_rom), config(config) {}

		void ExGraphics::extract() {
			spdlog::info(fmt::format(colors::RESOURCE, "Exporting ExGraphics"));

			const auto keep_symlink{ extracting_rom == config.output_rom.getOrThrow() };

			try {
				GraphicsUtil::exportProjectExGraphicsFrom(config, extracting_rom, keep_symlink);
			}
			catch (const std::exception& e) {
				throw ExtractionException(fmt::format(
					colors::EXCEPTION,
					"Failed to export ExGraphics from ROM {} with exception:\n\r{}",
					extracting_rom.string(),
					e.what()
				));
			}
		}
	}
}
