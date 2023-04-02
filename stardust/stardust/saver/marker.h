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
		static constexpr auto BITFIELD_LOCATION = COMMENT_ADDRESS + 8 + 16 * 6 + 7;

		static constexpr auto MARKER_STRING = 
			"        "
			"                "
			" {} "
			"                "
			"   Be curious   "
			"on your journey!"
			"                "
			"       \" : dw ${:04X} : db \"       "
			"                ";

		static uint16_t extractablesToBitField(const std::vector<ExtractableType>& extractables);
		static std::vector<ExtractableType> determineAddedExtractables(uint16_t old_extractables, uint16_t new_extractables);

		static std::string getMarkerString(const std::vector<ExtractableType>& extractables);

		static std::string getMarkerInsertionPatch(const std::vector<ExtractableType>& extractables);
		static std::string getMarkerCheckPatch();

		static std::optional<uint16_t> extractBitField(const fs::path& rom_path);

	public:
		static void insertMarkerString(const fs::path& rom_path, const std::vector<ExtractableType>& extractables);
		static std::vector<ExtractableType> getNeededExtractions(const fs::path& rom_path, 
			const std::vector<ExtractableType>& extractables, bool use_text_map16);
	};
}
