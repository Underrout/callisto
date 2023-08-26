#include "marker.h"

namespace callisto {
	uint16_t Marker::extractablesToBitField(const std::vector<ExtractableType>& extractables) {
		uint16_t bit_field{ 0 };
		for (const auto& extractable : extractables) {
			bit_field |= (1 << static_cast<size_t>(extractable));
		}
		return bit_field;
	}

	std::vector<ExtractableType> Marker::determineAddedExtractables(uint16_t old_extractables, uint16_t new_extractables) {
		uint16_t newly_added_bits{ static_cast<uint16_t>(~old_extractables & new_extractables) };
		std::vector<ExtractableType> extractables{};
		for (size_t i{ 0 }; static_cast<ExtractableType>(i) != ExtractableType::_COUNT; ++i) {
			if ((newly_added_bits & (1 << i)) != 0) {
				extractables.push_back(static_cast<ExtractableType>(i));
			}
		}
		return extractables;
	}

	std::string Marker::getMarkerString(const std::vector<ExtractableType>& extractables, int64_t timestamp) {
		const auto high_timestamp{ (timestamp & 0xFFFF000000) >> 24 };
		const auto low_timestamp{ timestamp & 0xFFFFFF };

		spdlog::debug("High marker timestamp is {:04X}", high_timestamp);
		spdlog::debug("Low marker timestamp is {:06X}", low_timestamp);

		return fmt::format(MARKER_STRING, CALLISTO_STRING, high_timestamp, low_timestamp, extractablesToBitField(extractables));
	}

	std::string Marker::getMarkerInsertionPatch(const std::vector<ExtractableType>& extractables, int64_t timestamp){
		return fmt::format("org ${:06X}\ndb \"{}\"", COMMENT_ADDRESS, getMarkerString(extractables, timestamp));
	}

	std::string Marker::getMarkerCheckPatch() {
		std::ostringstream patch{};

		patch << "!marker_present = 1" << std::endl;

		uint8_t byte;
		int i{ 0 };
		while ((byte = CALLISTO_STRING[i]) != '\0') {
			patch << fmt::format(
				"if read1(${:06X}) != ${:02X}\n!marker_present = 0\nendif\n",
				CALLISTO_STRING_LOCATION + i++, static_cast<int>(byte)
			);
		}
		patch << fmt::format("if !marker_present\n"
			"print hex(read2(${:06X}))\n"
			"print hex(read2(${:06X}))\n"
			"print hex(read3(${:06X}))\n"
			"endif", BITFIELD_LOCATION, TIMESTAMP_LOCATION, TIMESTAMP_LOCATION + 2);
		return patch.str();
	}

	std::optional<Marker::Extracted> Marker::extractInformation(const fs::path& rom_path) {
		std::ifstream rom_file(rom_path, std::ios::in | std::ios::binary);
		std::vector<char> rom_bytes((std::istreambuf_iterator<char>(rom_file)), (std::istreambuf_iterator<char>()));
		rom_file.close();

		int rom_size{ static_cast<int>(rom_bytes.size()) };
		const auto header_size{ rom_size & 0x7FFF };

		int unheadered_rom_size{ rom_size - header_size };

		const auto patch_string{ getMarkerCheckPatch() };

		if (!asar_init()) {
			spdlog::warn(fmt::format(colors::WARNING, "Failed to check ROM {} for marker string because asar not found", rom_path.string()));
		}

		asar_reset();

		const auto prev_diretcory{ fs::current_path() };
		fs::current_path(boost::filesystem::temp_directory_path().string());

		memoryfile patch;
		patch.path = "extractor.asm";
		patch.buffer = patch_string.data();
		patch.length = patch_string.size();

		const patchparams params{
			sizeof(struct patchparams),
			"extractor.asm",
			rom_bytes.data() + header_size,
			MAX_ROM_SIZE,
			&unheadered_rom_size,
			nullptr,
			0,
			true,
			nullptr,
			0,
			nullptr,
			nullptr,
			nullptr,
			0,
			&patch,
			1,
			true,
			false
		};

		const bool succeeded{ asar_patch_ex(&params) };

		fs::current_path(prev_diretcory);
		
		if (succeeded) {
			int print_count;
			const auto prints{ asar_getprints(&print_count) };
			if (print_count != 3) {
				spdlog::debug("Failed to extract marker information from ROM {}", rom_path.string());
				return {};
			}
			else {
				uint16_t bits;

				std::stringstream temp;
				temp << std::hex << prints[0];
				temp >> bits;

				uint64_t timestamp;
				temp.clear();
				temp << std::hex << prints[1] << prints[2];
				temp >> timestamp;

				spdlog::debug("Marker found in ROM {}, bitfield is 0x{:04X}, timestamp is 0x{:010X}", rom_path.string(), bits, timestamp);
				return Extracted(bits, timestamp);
			}
		}
		else {
			spdlog::warn(fmt::format(colors::WARNING, "Failed to check ROM {} for marker string", rom_path.string()));
			return {};
		}
	}

	void Marker::insertMarkerString(const fs::path& rom_path, const std::vector<ExtractableType>& extractables, int64_t timestamp) {
		std::ifstream rom_file(rom_path, std::ios::in | std::ios::binary);
		std::vector<char> rom_bytes((std::istreambuf_iterator<char>(rom_file)), (std::istreambuf_iterator<char>()));
		rom_file.close();

		int rom_size{ static_cast<int>(rom_bytes.size()) };
		const auto header_size{ rom_size & 0x7FFF };

		int unheadered_rom_size{ rom_size - header_size };

		const auto patch_string{ getMarkerInsertionPatch(extractables, timestamp) };

		if (!asar_init()) {
			spdlog::warn(fmt::format(colors::WARNING, "Failed to insert marker string into ROM {} because asar not found", rom_path.string()));
		}

		asar_reset();

		const auto prev_directory{ fs::current_path() };
		fs::current_path(boost::filesystem::temp_directory_path().string());

		memoryfile patch;
		patch.path = "inserter.asm";
		patch.buffer = patch_string.data();
		patch.length = patch_string.size();

		const patchparams params{
			sizeof(struct patchparams),
			"inserter.asm",
			rom_bytes.data() + header_size,
			MAX_ROM_SIZE,
			&unheadered_rom_size,
			nullptr,
			0,
			true,
			nullptr,
			0,
			nullptr,
			nullptr,
			nullptr,
			0,
			&patch,
			1,
			false,
			true
		};

		const bool succeeded{ asar_patch_ex(&params) };

		fs::current_path(prev_directory);

		if (succeeded) {
			spdlog::debug("Successfully inserted marker string into ROM {}", rom_path.string());

			std::ofstream out_rom{ rom_path, std::ios::out | std::ios::binary };
			out_rom.write(rom_bytes.data(), rom_bytes.size());
			out_rom.close();
		}
		else {
			spdlog::warn(fmt::format(colors::WARNING, "Failed to insert marker string into ROM {}", rom_path.string()));
		}
	}

	std::vector<ExtractableType> Marker::getNeededExtractions(const fs::path& rom_path, const fs::path& project_root,
		const std::vector<ExtractableType>& extractables, bool use_text_map16) {

		const auto last_rom_sync_path{ PathUtil::getLastRomSyncPath(project_root) };
		if (!fs::exists(last_rom_sync_path)) {
			// Don't know when ROM was last synced with project files, assume worst case and export all
			return extractables;
		}

		const auto extracted_information{ extractInformation(rom_path) };
		if (!extracted_information.has_value()) {
			// Marker not found, export all
			return extractables;
		}

		const auto rom_write_time{ std::chrono::clock_cast<std::chrono::system_clock>(fs::last_write_time(rom_path)) };
		const auto rom_epoch{ std::chrono::duration_cast<std::chrono::seconds>(rom_write_time.time_since_epoch()).count() };
		
		if (rom_epoch != extracted_information.value().timestamp) {
			// Marker timestamp does not match last ROM write time, export all
			return extractables;
		}

		std::ifstream last_sync_file{ last_rom_sync_path };
		const auto last_sync_json{ json::parse(last_sync_file) };
		const auto last_sync_time{ last_sync_json["last_sync_time"] };
		last_sync_file.close();

		if (rom_epoch != last_sync_time) {
			// ROM's last modified time & marker timestamp mismatch with our last recorded sync time,
			// our ROM must have been replaced by another ROM, export all
			return extractables;
		}

		return determineAddedExtractables(extracted_information.value().bitfield, extractablesToBitField(extractables));
	}
}
