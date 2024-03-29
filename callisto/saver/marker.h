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
#include <nlohmann/json.hpp>

#include "extractable_type.h"
#include "../path_util.h"

#include "../colors.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace callisto {
	class Marker {
	protected:
		static constexpr auto MAX_ROM_SIZE = 16 * 1024 * 1024;
		static constexpr auto COMMENT_ADDRESS = 0xFF0B8;

		static constexpr auto CALLISTO_STRING = "-~*callisto*~-";
		static constexpr auto CALLISTO_STRING_LOCATION = COMMENT_ADDRESS + 8 + 16 + 1;
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
			const long long timestamp;

			Extracted(uint16_t bitfield, long long timestamp) : bitfield(bitfield), timestamp(timestamp) {}
		};

		static uint16_t extractablesToBitField(const std::vector<ExtractableType>& extractables);
		static std::vector<ExtractableType> determineAddedExtractables(uint16_t old_extractables, uint16_t new_extractables);

		static std::string getMarkerString(const std::vector<ExtractableType>& extractables, int64_t timestamp);

		static std::string getMarkerInsertionPatch(const std::vector<ExtractableType>& extractables, int64_t timestamp);
		static std::string getMarkerCheckPatch();

		static std::optional<Extracted> extractInformation(const fs::path& rom_path);

	public:
		static void insertMarkerString(const fs::path& rom_path, const std::vector<ExtractableType>& extractables, int64_t timestamp);
		static std::vector<ExtractableType> getNeededExtractions(const fs::path& rom_path, const fs::path& project_root,
			const std::vector<ExtractableType>& extractables, bool use_text_map16);
	};
}
