#pragma once

#include <filesystem>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_extractable.h"
#include "extraction_exception.h"
#include "../not_found_exception.h"

#include "../configuration/configuration.h"
#include "../dependency/policy.h"

namespace fs = std::filesystem;

namespace stardust {
	namespace extractables {
		class ExGraphics : public LunarMagicExtractable {
		protected:
			static constexpr auto EXGRAPHICS_FOLDER_NAME = "ExGraphics";

			const fs::path source_exgraphics_folder;
			const fs::path target_exgraphics_folder;

		public:
			void extract() override;

			ExGraphics(const Configuration& config, const fs::path& extracting_rom);
		};
	}
}
