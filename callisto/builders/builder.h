#pragma once

#include <memory>

#include <nlohmann/json.hpp>

#include "../configuration/configuration.h"

#include "../colors.h"

#include "../insertables/initial_patch.h"
#include "../insertables/exgraphics.h"
#include "../insertables/graphics.h"
#include "../insertables/shared_palettes.h"
#include "../insertables/overworld.h"
#include "../insertables/title_screen.h"
#include "../insertables/global_exanimation.h"
#include "../insertables/credits.h"
#include "../insertables/title_moves.h"
#include "../insertables/levels.h"
#include "../insertables/binary_map16.h"
#include "../insertables/text_map16.h"
#include "../insertables/external_tool.h"
#include "../insertables/patch.h"
#include "../insertables/module.h"

#include "../insertable.h"
#include "../saver/saver.h"
#include "../descriptor.h"

#include "../time_util.h"
#include "../prompt_util.h"

using json = nlohmann::json;

namespace callisto {
	class Builder {
	protected:
		static constexpr auto BUILD_REPORT_VERSION{ 1 };

		static constexpr auto DEFINE_PREFIX{ "CALLISTO" };
		static constexpr auto VERSION_DEFINE_NAME{ "VERSION" };
		static constexpr auto MAJOR_VERSION_DEFINE_NAME{ "MAJOR" };
		static constexpr auto MINOR_VERSION_DEFINE_NAME{ "MINOR" };
		static constexpr auto PATCH_VERSION_DEFINE_NAME{ "PATCH" };
		static constexpr auto ASSEMBLING_DEFINE_NAME{ "ASSEMBLING" };
		static constexpr auto INCLUDED_DEFINE_NAME{ "INCLUDED" };
		static constexpr auto PROFILE_DEFINE_NAME{ "PROFILE" };
		static constexpr auto MODULE_FOLDER_PATH_DEFINE_NAME{ "MODULES" };
		static constexpr auto CALLISTO_FOLDER_PATH_DEFINE_NAME{ "PATH" };
		static constexpr auto PROJECT_ROOT_PATH_DEFINE_NAME{ "ROOT" };
		static constexpr auto CALLISTO_HEADER_PATH_DEFINE_NAME{ "HEADER" };

		static constexpr auto CHECKSUM_LOCATION{ 0x7FDE };
		static constexpr auto CLEAN_ROM_CHECKSUM{ 0xA0DA };

		static constexpr auto CHECKSUM_COMPLEMENT_LOCATION{ 0x7FDC };
		static constexpr auto CLEAN_ROM_CHECKSUM_COMPLEMENT{ CLEAN_ROM_CHECKSUM ^ 0xFFFF };

		static constexpr auto CLEAN_ROM_SIZE{ 0x80000 };
		static constexpr auto HEADER_SIZE{ 0x200 };

		using Insertables = std::vector<std::pair<Descriptor, std::shared_ptr<Insertable>>>;
		using DependencyVector = std::vector<std::pair<Descriptor, std::pair<std::unordered_set<ResourceDependency>,
			std::unordered_set<ConfigurationDependency>>>>;

		std::shared_ptr<std::unordered_set<int>> module_addresses{ std::make_shared<std::unordered_set<int>>() };
		int module_count{ 0 };
	
		Insertables buildOrderToInsertables(const Configuration& config);
		std::shared_ptr<Insertable> descriptorToInsertable(const Descriptor& descriptor, const Configuration& config);

		static json createBuildReport(const Configuration& config, const json& dependency_report);
		static void writeBuildReport(const fs::path& project_root, const json& j);

		static void cacheModules(const fs::path& project_root);
		static void moveTempToOutput(const Configuration& config);

		void init(const Configuration& config);
		static void ensureCacheStructure(const Configuration& config);
		static void generateCallistoAsmFile(const Configuration& config);

		static void tryConvenienceSetup(const Configuration& config);
		static void convenienceSetup(const Configuration& config);

		static inline void createFolderStructureFor(const PathConfigVariable& var) {
			if (var.isSet()) {
				fs::create_directories(var.getOrThrow().parent_path());
			}
		}

		static void checkCleanRom(const fs::path& clean_rom_path);

		static void writeIfDifferent(const std::string& str, const fs::path& out_file);

		static void removeBuildReport(const fs::path& project_root);
	};
}
