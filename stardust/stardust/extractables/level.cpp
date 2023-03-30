#include "level.h"

namespace stardust {
	namespace extractables {
		Level::Level(const Configuration& config, const fs::path& mwl_file, int level_number)
			: LunarMagicExtractable(config), mwl_file(mwl_file), level_number(level_number) {
			if (!fs::exists(mwl_file.parent_path())) {
				spdlog::debug("Levels directory {} does not exist, creating it now", mwl_file.parent_path().string());

				try {
					fs::create_directories(mwl_file.parent_path());
				}
				catch (const fs::filesystem_error&) {
					throw ExtractionException(fmt::format(
						"Failed to create levels directory {}",
						mwl_file.parent_path().string()
					));
				}
			}
		}

		unsigned int Level::fourBytesToInt(const std::vector<uint8_t>& bytes, unsigned int offset) {
			return bytes.at(offset + 3) << 24 | bytes.at(offset + 2) << 16
				| bytes.at(offset + 1) << 8 | bytes.at(offset);
		}

		void Level::extract() {
			spdlog::info("Exporting level {} to file {}", level_number, mwl_file.string());
			spdlog::debug("Exporting level {} from ROM {} to file {}",
				level_number, extracting_rom.string(), mwl_file.string());

			const auto exit_code{ callLunarMagic("-ExportLevel", extracting_rom.string(),
				mwl_file.string(), std::to_string(level_number)) };

			if (exit_code == 0) {
				if (strip_source_pointers) {
					stripSourcePointers(mwl_file);
				}
				spdlog::info("Successfully exported level {} to {}", level_number, mwl_file.string());
			}
			else {
				throw ExtractionException(fmt::format(
					"Failed to export level {} from ROM {} to {}", level_number, extracting_rom.string(), mwl_file.string()
				));
			}
		}

		void Level::stripSourcePointers(const fs::path& mwl_path) {
			spdlog::debug("Stripping layer 1, layer 2, sprite and palette source addresses from MWL file {}", mwl_path.string());

			std::ifstream mwl_file(mwl_path, std::ios::in | std::ios::binary);
			std::vector<uint8_t> mwl_bytes((std::istreambuf_iterator<char>(mwl_file)), (std::istreambuf_iterator<char>()));
			mwl_file.close();

			const auto data_pointer_table_offset{ fourBytesToInt(mwl_bytes, MWL_HEADER_POINTER_OFFSET) };
			
			for (const auto& data_pointer_offset : MWL_DATA_POINTER_OFFSETS) {
				const auto data_start_offset{ fourBytesToInt(mwl_bytes, data_pointer_table_offset + data_pointer_offset) };
				const auto source_address_offset{ data_start_offset + MWL_SOURCE_ADDRESS_OFFSET };
				const auto source_address{ fourBytesToInt(mwl_bytes, source_address_offset) & 0xFFFFFF };
				if (source_address >> 16 != MLW_NO_STRIP_BANK_BYTE) {
					for (size_t i{ 0 }; i != MWL_SOURCE_ADDRES_SIZE; ++i) {
						mwl_bytes[source_address_offset + i] = 0x0;
					}
					spdlog::debug("Stripped source address {:06X} at {:08X} in MWL file {}", 
						source_address, source_address_offset, mwl_path.string());
				}
				else {
					spdlog::debug("Source address {:06X} at {:06X} in MWL file {} points to original game data, skipping it",
						source_address, source_address_offset, mwl_path.string());
				}
			}

			std::ofstream out_mwl_file(mwl_path, std::ios::out | std::ios::binary);
			out_mwl_file.write(reinterpret_cast<const char*>(mwl_bytes.data()), mwl_bytes.size());
			out_mwl_file.close();
		}
	}
}
