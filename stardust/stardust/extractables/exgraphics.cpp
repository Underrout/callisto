#include "exgraphics.h"

namespace stardust {
	namespace extractables {
		ExGraphics::ExGraphics(const Configuration& config, const fs::path& extracting_rom)
			: LunarMagicExtractable(config, extracting_rom), config(config) {}

		void ExGraphics::extract() {
			spdlog::info("Exporting ExGraphics");

			try {
				GraphicsUtil::exportProjectExGraphicsFrom(config, extracting_rom);
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
