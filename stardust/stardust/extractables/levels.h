#pragma once

#include <filesystem>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "lunar_magic_extractable.h"
#include "extraction_exception.h"
#include "../not_found_exception.h"
#include "level.h"

namespace fs = std::filesystem;

namespace stardust {
	namespace extractables {
		class Levels : public LunarMagicExtractable {
		protected:
			const fs::path levels_folder;
			const bool strip_source_pointers{ true };  // TODO potentially make this configurable?

			void normalize() const;

		public:
			void extract() override;

			Levels(const Configuration& config, const fs::path& extracting_rom);
		};
	}
}
