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
		class Graphics : public LunarMagicExtractable {
		protected:
			static constexpr auto GRAPHICS_FOLDER_NAME = "Graphics";

			const fs::path source_graphics_folder;
			const fs::path target_graphics_folder;

		public:
			void extract() override;

			Graphics(const Configuration& config, const fs::path& extracting_rom);
		};
	}
}
