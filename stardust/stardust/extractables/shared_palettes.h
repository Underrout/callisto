#pragma once

#include <filesystem>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "lunar_magic_extractable.h"
#include "extraction_exception.h"

namespace fs = std::filesystem;

namespace stardust {
	namespace extractables {
		class SharedPalettes : public LunarMagicExtractable {
		protected:
			const fs::path shared_palettes_path;

		public:
			void extract() override;

			SharedPalettes(const Configuration& config, const fs::path& extracting_rom);
		};
	}
}
