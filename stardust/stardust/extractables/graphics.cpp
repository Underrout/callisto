#include "graphics.h"

namespace stardust {
	namespace extractables {
		Graphics::Graphics(const Configuration& config, const fs::path& extracting_rom)
			: LunarMagicExtractable(config, extracting_rom), config(config) {}

		void Graphics::extract() {
			spdlog::info("Exporting Graphics");

			const auto keep_symlink{ extracting_rom == config.project_rom.getOrThrow() };

			try {
				GraphicsUtil::exportProjectGraphicsFrom(config, extracting_rom, keep_symlink);
			}
			catch (const std::exception&) {
				throw ExtractionException(fmt::format(
					"Failed to export Graphics from ROM {}",
					extracting_rom.string()
				));
			}
		}
	}
}
