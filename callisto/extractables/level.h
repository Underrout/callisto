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

namespace callisto {
	namespace extractables {
		class Level : public LunarMagicExtractable {
		protected:
			static constexpr auto MWL_L1_DATA_POINTER_OFFSET{ 0x8 };
			static constexpr auto MWL_L2_DATA_POINTER_OFFSET{ 0x10 };
			static constexpr std::array<size_t, 4> MWL_DATA_POINTER_OFFSETS{ MWL_L1_DATA_POINTER_OFFSET, MWL_L2_DATA_POINTER_OFFSET, 0x18, 0x20 };
			static constexpr auto MWL_HEADER_POINTER_OFFSET{ 0x4 };
			static constexpr auto MWL_SOURCE_ADDRESS_OFFSET{ 0x4 };
			static constexpr auto MWL_SOURCE_ADDRES_SIZE{ 0x3 };
			static constexpr auto MLW_NO_STRIP_BANK_BYTE{ 0xFF };

			const fs::path mwl_file;
			const int level_number;
			const bool strip_source_pointers{ true };  // TODO potentially make this configurable?

			static unsigned int fourBytesToInt(const std::vector<unsigned char>& bytes, unsigned int offset);

			static unsigned long long bytesToInt(const std::vector<unsigned char>& bytes, unsigned int offset, size_t number);

			static std::pair<bool, uint8_t> determineSkip(const std::vector<unsigned char>& bytes, unsigned int offset);

			static uint8_t getStandardObjectId(const std::vector<unsigned char>& bytes, unsigned int offset);
			static uint8_t getExtendedObjectNumber(const std::vector<unsigned char>& bytes, unsigned int offset);
			static uint8_t get27_29Size(const std::vector<unsigned char>& bytes, unsigned int offset);

			static void writeFourBytes(std::vector<unsigned char>& mwl, unsigned int offset, unsigned int bytes);
			static void writeFiveBytes(std::vector<unsigned char>& mwl, unsigned int offset, unsigned int bytes);

			static bool canStrip(size_t data_pointer_offset, size_t source_address, size_t data_start_offset, std::vector<unsigned char>& mwl);


		public:
			void extract() override;

			static void normalize(const fs::path& mwl_path);

			Level(const Configuration& config, const fs::path& mwl_file, int level_number, const fs::path& extracting_rom);
		};
	}
}
