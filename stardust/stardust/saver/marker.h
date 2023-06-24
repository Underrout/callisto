#pragma once

#include <vector>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>

#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <asar-dll-bindings/c/asardll.h>
#include <boost/filesystem.hpp>

#include "extractable_type.h"

namespace fs = std::filesystem;

namespace stardust {
	class Marker {
	protected:
		static constexpr auto MAX_ROM_SIZE = 16 * 1024 * 1024;
		static constexpr auto COMMENT_ADDRESS = 0xFF0B8;

		static constexpr auto STARDUST_STRING = "-~*stardust*~-";
		static constexpr auto STARDUST_STRING_LOCATION = COMMENT_ADDRESS + 8 + 16 + 1;
		static constexpr auto TIMESTAMP_LOCATION = COMMENT_ADDRESS + 8 + 16 * 6 + 4;
		static constexpr auto BITFIELD_LOCATION = TIMESTAMP_LOCATION + 6;

		static constexpr auto MARKER_STRING = 
			"        "
			"                "
			" {} "
			"                "
			"   Be curious   "
			"on your journey!"
			"                "
			"    \" : dw ${:04X} : dl ${:06X} : db \" \" : dw ${:04X} : db \"    "
			"                ";

		struct Extracted {
			const uint16_t bitfield;
			const uint64_t timestamp;

			Extracted(uint16_t bitfield, uint64_t timestamp) : bitfield(bitfield), timestamp(timestamp) {}
		};

		static uint16_t extractablesToBitField(const std::vector<ExtractableType>& extractables);
		static std::vector<ExtractableType> determineAddedExtractables(uint16_t old_extractables, uint16_t new_extractables);

		static std::string getMarkerString(const std::vector<ExtractableType>& extractables, int64_t timestamp);

		static std::string getMarkerInsertionPatch(const std::vector<ExtractableType>& extractables, int64_t timestamp);
		static std::string getMarkerCheckPatch();

		static std::optional<Extracted> extractInformation(const fs::path& rom_path);

	public:
		static void insertMarkerString(const fs::path& rom_path, const std::vector<ExtractableType>& extractables, int64_t timestamp);
		static std::vector<ExtractableType> getNeededExtractions(const fs::path& rom_path, 
			const std::vector<ExtractableType>& extractables, bool use_text_map16);
	};
}
