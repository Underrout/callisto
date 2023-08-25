#pragma once

#include <filesystem>
#include <algorithm>
#include <execution>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "lunar_magic_extractable.h"
#include "extraction_exception.h"
#include "../not_found_exception.h"
#include "level.h"

namespace fs = std::filesystem;

namespace callisto {
	namespace extractables {
		class Levels : public LunarMagicExtractable {
		protected:
			static constexpr auto LAYER_1_POINTERS_OFFSET{ 0x2E000 };
			static constexpr auto LAYER_1_POINTER_SIZE{ 3 };
			static constexpr auto LAYER_1_POINTERS_AMOUNT{ 0x200 };
			
			const fs::path levels_folder;
			const bool strip_source_pointers{ true };  // TODO potentially make this configurable?
			const fs::path temp_folder;
			const size_t max_thread_count;

			void normalize() const;
			
			std::vector<size_t> determineModifiedOffsets(const fs::path& extracting_rom);
			fs::path createChunkedRom(const fs::path& temp_folder, size_t chunk_idx, size_t chunk_amount,
				const fs::path& extracting_rom, const std::vector<size_t>& offsets);

		public:
			void extract() override;

			Levels(const Configuration& config, const fs::path& extracting_rom, size_t max_thread_count);
		};
	}
}
