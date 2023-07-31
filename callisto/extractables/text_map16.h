#pragma once

#include <filesystem>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_extractable.h"
#include "extraction_exception.h"
#include "../not_found_exception.h"

#include "../human_map16/human_readable_map16.h"
#include "../human_map16/human_map16_exception.h"

#include "../configuration/configuration.h"

namespace fs = std::filesystem;

namespace callisto {
	namespace extractables {
		class TextMap16 : public LunarMagicExtractable {
		protected:
			const fs::path map16_folder_path;

			fs::path getTemporaryMap16FilePath() const;
			void exportTemporaryMap16File() const;
			void deleteTemporaryMap16File() const;

		public:
			void extract() override;

			TextMap16(const Configuration& config, const fs::path& extracting_rom);
		};
	}
}
