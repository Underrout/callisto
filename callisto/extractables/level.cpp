#include "level.h"

namespace callisto {
	namespace extractables {
		Level::Level(const Configuration& config, const fs::path& mwl_file, int level_number, const fs::path& extracting_rom)
			: LunarMagicExtractable(config, extracting_rom), mwl_file(mwl_file), level_number(level_number) {
			if (!fs::exists(mwl_file.parent_path())) {
				spdlog::debug("Levels directory {} does not exist, creating it now", mwl_file.parent_path().string());

				try {
					fs::create_directories(mwl_file.parent_path());
				}
				catch (const fs::filesystem_error& e) {
					throw ExtractionException(fmt::format(
						colors::EXCEPTION,
						"Failed to create levels directory {} with exception:\n\r{}",
						mwl_file.parent_path().string(),
						e.what()
					));
				}
			}
		}

		// little endian
		unsigned int Level::fourBytesToInt(const std::vector<uint8_t>& bytes, unsigned int offset) {
			return bytes.at(offset + 3) << 24 | bytes.at(offset + 2) << 16
				| bytes.at(offset + 1) << 8 | bytes.at(offset);
		}

		// big endian
		unsigned long long Level::bytesToInt(const std::vector<unsigned char>& bytes, unsigned int offset, size_t number) {
			long long comb{ 0 };
			for (size_t i{ 0 }; i != number; ++i) {
				comb |= static_cast<long long>(bytes.at(offset + i)) << ((number - i - 1) * 8);
			}
			return comb;
		}

		std::pair<bool, uint8_t> Level::determineSkip(const std::vector<unsigned char>& bytes, unsigned int offset) {
			const auto standard_id{ getStandardObjectId(bytes, offset) };

			if (standard_id == 0x0) {
				const auto extended_no{ getExtendedObjectNumber(bytes, offset) };
				if (extended_no == 0x0) {
					// screen exit
					return { true, 4 };
				}
				if (extended_no == 0x2) {
					// extended screen exit
					return { true, 5 };
				}
				// normal extended object
				return { false, 3 };
			}
			if (standard_id == 0x22 || standard_id == 0x23) {
				return { false, 4 };
			}
			if (standard_id == 0x24 || standard_id == 0x25) {
				return { false, 4 };
			}
			if (standard_id == 0x26) {
				return { false, 3 };
			}
			if (standard_id == 0x27 || standard_id == 0x29) {
				return { false, get27_29Size(bytes, offset) };
			}
			if (standard_id == 0x28) {
				return { false, 3 } ;
			}
			if (standard_id == 0x2D) {
				return { false, 5 };
			}
			return { false, 3 };
		}

		uint8_t Level::getStandardObjectId(const std::vector<unsigned char>& bytes, unsigned int offset) {
			const auto two{ bytesToInt(bytes, offset, 2) };
			const auto high{ (two & 0x6000) >> 9 };
			const auto low{ (two & 0x00F0) >> 4 };
			return high | low;
		}

		uint8_t Level::getExtendedObjectNumber(const std::vector<unsigned char>& bytes, unsigned int offset) {
			return bytesToInt(bytes, offset + 2, 1);
		}

		uint8_t Level::get27_29Size(const std::vector<unsigned char>& bytes, unsigned int offset) {
			const auto rel_byte_1{ bytesToInt(bytes, offset + 3, 1) };
			const auto masked{ (rel_byte_1 & 0b11000000) >> 6 };
			if (masked != 0b11) {
				if (masked == 0b00) {
					return 5;
				}
				else if (masked == 0b01) {
					return 5;
				}
				else {
					return 6;
				}
			}
			else {
				const auto rel_byte_2{ bytesToInt(bytes, offset + 2, 1) };
				const auto masked_2{ (rel_byte_2 & 0b10000000) >> 7 };
				if (masked_2 == 0b0) {
					return 7;
				}
				else {
					return 8;
				}
			}
		}

		void Level::writeFourBytes(std::vector<unsigned char>& mwl, unsigned int offset, unsigned int bytes) {
			mwl[offset] = (bytes >> 24) & 0xFF;
			mwl[offset + 1] = (bytes >> 16) & 0xFF;
			mwl[offset + 2] = (bytes >> 8) & 0xFF;
			mwl[offset + 3] = bytes & 0xFF;
		}

		void Level::writeFiveBytes(std::vector<unsigned char>& mwl, unsigned int offset, unsigned int bytes) {
			mwl[offset] = (bytes >> 32) & 0xFF;
			mwl[offset + 1] = (bytes >> 24) & 0xFF;
			mwl[offset + 2] = (bytes >> 16) & 0xFF;
			mwl[offset + 3] = (bytes >> 8) & 0xFF;
			mwl[offset + 4] = bytes & 0xFF;
		}

		void Level::extract() {
			spdlog::info(fmt::format(colors::RESOURCE, "Exporting level {} to file {}", level_number, mwl_file.string()));
			spdlog::debug("Exporting level {} from ROM {} to file {}",
				level_number, extracting_rom.string(), mwl_file.string());

			const auto exit_code{ callLunarMagic("-ExportLevel", extracting_rom.string(),
				mwl_file.string(), std::to_string(level_number)) };

			if (exit_code == 0) {
				if (strip_source_pointers) {
					normalize(mwl_file);
				}
				spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Successfully exported level {} to {}", level_number, mwl_file.string()));
			}
			else {
				throw ExtractionException(fmt::format(
					colors::EXCEPTION,
					"Failed to export level {} from ROM {} to {}", level_number, extracting_rom.string(), mwl_file.string()
				));
			}
		}

		void Level::normalize(const fs::path& mwl_path) {
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

			std::vector<long long> screen_exits_four{};
			std::vector<unsigned int> screen_exit_indices_four{};
			std::vector<bool> has_new_screen_set_four{};
			std::vector<unsigned int> screen_exits_five{};
			std::vector<unsigned int> screen_exit_indices_five{};
			std::vector<bool> has_new_screen_set_five{};
			auto curr_offset{ fourBytesToInt(mwl_bytes, data_pointer_table_offset + MWL_L1_DATA_POINTER_OFFSET) + 13 };
			while (bytesToInt(mwl_bytes, curr_offset, 1) != 0xFF) {
				const auto skippage{ determineSkip(mwl_bytes, curr_offset) };
				const auto& is_screen_exit{ skippage.first };
				const auto& skip_value{ skippage.second };

				if (is_screen_exit) {
					spdlog::debug("Found screen exit at 0x{:06X} of length {} in {}", curr_offset, skip_value, mwl_path.string());
					auto val{ bytesToInt(mwl_bytes, curr_offset, skip_value) };
					const auto new_flag_mask{ 0x80 << ((skip_value - 1) * 8) };
					bool is_new_screen{ false };
					if ((val & new_flag_mask) != 0) {
						is_new_screen = true;
					}
					val &= ~new_flag_mask;
					if (skip_value == 4) {
						screen_exits_four.push_back(val);
						screen_exit_indices_four.push_back(curr_offset);
						has_new_screen_set_four.push_back(is_new_screen);
					}
					else {
						screen_exits_five.push_back(val);
						screen_exit_indices_five.push_back(curr_offset);
						has_new_screen_set_five.push_back(is_new_screen);
					}
				}
				curr_offset += skip_value;
			}

			std::sort(screen_exits_four.begin(), screen_exits_four.end());
			std::sort(screen_exits_five.begin(), screen_exits_five.end());

			size_t i{ 0 };
			for (const auto& exit : screen_exits_four) {
				const auto masked{ !has_new_screen_set_four.at(i) ? exit : exit | 0x80000000 };
				writeFourBytes(mwl_bytes, screen_exit_indices_four.at(i++), masked);
			}

			i = 0;
			for (const auto& exit : screen_exits_five) {
				const auto masked{ !has_new_screen_set_five.at(i) ? exit : exit | 0x8000000000 };
				writeFourBytes(mwl_bytes, screen_exit_indices_five.at(i++), masked);
			}

			std::ofstream out_mwl_file(mwl_path, std::ios::out | std::ios::binary);
			out_mwl_file.write(reinterpret_cast<const char*>(mwl_bytes.data()), mwl_bytes.size());
			out_mwl_file.close();
		}
	}
}
