#pragma once

#include <filesystem>
#include <vector>
#include <memory>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "marker.h"

#include "../configuration/configuration.h"
#include "extractable_type.h"
#include "../extractables/extractable.h"

#include "../extractables/binary_map16.h"
#include "../extractables/credits.h"
#include "../extractables/global_exanimation.h"
#include "../extractables/overworld.h"
#include "../extractables/shared_palettes.h"
#include "../extractables/text_map16.h"
#include "../extractables/title_screen.h"
#include "../extractables/levels.h"

#include "../symbol.h"

#include "../stardust_exception.h"

#include "../path_util.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace stardust {
	class Saver {
	protected:
		static const std::unordered_map<ExtractableType, Symbol> extractable_to_symbol;

		static std::vector<ExtractableType> getExtractableTypes(const Configuration& config);
		static std::shared_ptr<Extractable> extractableTypeToExtractable(const Configuration& config, ExtractableType type);
		static std::vector<std::shared_ptr<Extractable>> getExtractables(const Configuration& config, 
			const std::vector<ExtractableType>& extractable_types);
		static void updateBuildReport(const fs::path& build_report, const std::vector<ExtractableType>& extracted_types);

	public:
		static void writeMarkerToRom(const fs::path& rom_path, const Configuration& config);
		static void exportResources(const fs::path& rom_path, const Configuration& config);
	};
}
