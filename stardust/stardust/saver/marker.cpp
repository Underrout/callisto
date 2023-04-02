#include "marker.h"

namespace stardust {
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

	std::string Marker::getMarkerString(const std::vector<ExtractableType>& extractables) {
		return fmt::format(MARKER_STRING, STARDUST_STRING, extractablesToBitField(extractables));
	}

	std::string Marker::getMarkerInsertionPatch(const std::vector<ExtractableType>& extractables){
		return fmt::format("org ${:06X}\ndb \"{}\"", COMMENT_ADDRESS, getMarkerString(extractables));
	}

	std::string Marker::getMarkerCheckPatch() {
		std::ostringstream patch{};

		patch << "!marker_present = 1" << std::endl;

		uint8_t byte;
		int i{ 0 };
		while ((byte = STARDUST_STRING[i]) != '\0') {
			patch << fmt::format(
				"if read1(${:06X}) != ${:02X}\n!marker_present = 0\nendif\n",
				STARDUST_STRING_LOCATION + i++, static_cast<int>(byte)
			);
		}
		patch << fmt::format("if !marker_present\nprint hex(read2(${:06X}))\nendif", BITFIELD_LOCATION);
		return patch.str();
	}

	std::optional<uint16_t> Marker::extractBitField(const fs::path& rom_path) {
		std::ifstream rom_file(rom_path, std::ios::in | std::ios::binary);
		std::vector<char> rom_bytes((std::istreambuf_iterator<char>(rom_file)), (std::istreambuf_iterator<char>()));
		rom_file.close();

		int rom_size{ static_cast<int>(rom_bytes.size()) };
		const auto header_size{ rom_size & 0x7FFF };

		int unheadered_rom_size{ rom_size - header_size };

		const auto patch_string{ getMarkerCheckPatch() };

		if (!asar_init()) {
			spdlog::warn("Failed to check ROM {} for marker string because asar not found", rom_path.string());
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
			false,
			true
		};

		const bool succeeded{ asar_patch_ex(&params) };

		fs::current_path(prev_diretcory);
		
		if (succeeded) {
			int print_count;
			const auto prints{ asar_getprints(&print_count) };
			if (print_count == 0) {
				spdlog::debug("No marker string found in ROM {}", rom_path.string());
				return {};
			}
			else {
				int bits;
				std::stringstream temp;
				temp << std::hex << prints[0];
				temp >> bits;
				spdlog::debug("Marker found in ROM {}, bitfield is 0x{:04X}", rom_path.string(), bits);
				return bits;
			}
		}
		else {
			spdlog::warn("Failed to check ROM {} for marker string", rom_path.string());
			return {};
		}
	}

	std::vector<ExtractableType> Marker::getAllExtractables(bool use_text_map16) {
		std::vector<ExtractableType> all_extractables{};
		for (size_t i{ 0 }; static_cast<ExtractableType>(i) != ExtractableType::_COUNT; ++i) {
			const auto extractable_type{ static_cast<ExtractableType>(i) };
			if (extractable_type != ExtractableType::BINARY_MAP16 || !use_text_map16) {
				all_extractables.push_back(extractable_type);
			}
		}
		return all_extractables;
	}

	void Marker::insertMarkerString(const fs::path& rom_path, const std::vector<ExtractableType>& extractables) {
		std::ifstream rom_file(rom_path, std::ios::in | std::ios::binary);
		std::vector<char> rom_bytes((std::istreambuf_iterator<char>(rom_file)), (std::istreambuf_iterator<char>()));
		rom_file.close();

		int rom_size{ static_cast<int>(rom_bytes.size()) };
		const auto header_size{ rom_size & 0x7FFF };

		int unheadered_rom_size{ rom_size - header_size };

		const auto patch_string{ getMarkerInsertionPatch(extractables) };

		// need to call this again since PIXI might have called asar_close
		if (!asar_init()) {
			spdlog::warn("Failed to insert marker string into ROM {} because asar not found", rom_path.string());
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
			spdlog::warn("Failed to insert marker string into ROM {}", rom_path.string());
		}
	}

	std::vector<ExtractableType> Marker::getNeededExtractions(const fs::path& rom_path, 
		const std::vector<ExtractableType>& extractables, bool use_text_map16) {
		const auto bitfield{ extractBitField(rom_path) };
		if (!bitfield.has_value()) {
			return getAllExtractables(use_text_map16);
		}

		return determineAddedExtractables(bitfield.value(), extractablesToBitField(extractables));
	}
}
