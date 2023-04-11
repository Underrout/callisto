#include "exgraphics.h"

namespace stardust {
	namespace extractables {
		ExGraphics::ExGraphics(const Configuration& config, const fs::path& extracting_rom)
			: LunarMagicExtractable(config, extracting_rom), config(config) {}

		void ExGraphics::extract() {
			spdlog::info("Exporting ExGraphics");

			const auto keep_symlink{ extracting_rom == config.project_rom.getOrThrow() };

			try {
				GraphicsUtil::exportProjectExGraphicsFrom(config, extracting_rom, keep_symlink);
			}
			catch (const std::exception&) {
				throw ExtractionException(fmt::format(
					"Failed to export ExGraphics from ROM {}",
					extracting_rom.string()
				));
			}
		}
	}
}
