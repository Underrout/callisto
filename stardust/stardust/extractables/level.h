#pragma once

#include <filesystem>
#include <array>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "lunar_magic_extractable.h"
#include "extraction_exception.h"
#include "../not_found_exception.h"

#include "../configuration/configuration.h"

namespace fs = std::filesystem;

namespace stardust {
	namespace extractables {
		class Level : public LunarMagicExtractable {
		protected:
			static constexpr std::array<size_t, 4> MWL_DATA_POINTER_OFFSETS{ 0x8, 0x10, 0x18, 0x20 };
			static constexpr auto MWL_HEADER_POINTER_OFFSET{ 0x4 };
			static constexpr auto MWL_SOURCE_ADDRESS_OFFSET{ 0x4 };
			static constexpr auto MWL_SOURCE_ADDRES_SIZE{ 0x3 };
			static constexpr auto MLW_NO_STRIP_BANK_BYTE{ 0xFF };

			const fs::path mwl_file;
			const int level_number;
			const bool strip_source_pointers{ true };  // TODO potentially make this configurable?

			static unsigned int fourBytesToInt(const std::vector<unsigned char>& bytes, unsigned int offset);

		public:
			void extract() override;

			static void stripSourcePointers(const fs::path& mwl_path);

			Level(const Configuration& config, const fs::path& mwl_file, int level_number, const fs::path& extracting_rom);
		};
	}
}
