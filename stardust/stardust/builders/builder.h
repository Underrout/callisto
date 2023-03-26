#pragma once

#include <memory>

#include <nlohmann/json.hpp>

#include "../configuration/configuration.h"

#include "../insertables/exgraphics.h"
#include "../insertables/graphics.h"
#include "../insertables/shared_palettes.h"
#include "../insertables/overworld.h"
#include "../insertables/title_screen.h"
#include "../insertables/global_exanimation.h"
#include "../insertables/credits.h"
#include "../insertables/title_moves.h"
#include "../insertables/level.h"
#include "../insertables/binary_map16.h"
#include "../insertables/text_map16.h"
#include "../insertables/external_tool.h"
#include "../insertables/patch.h"
#include "../insertables/globule.h"
#include "../insertables/pixi.h"

#include "../insertable.h"
#include "../descriptor.h"

using json = nlohmann::json;

namespace stardust {
	class Builder {
	protected:
		static constexpr auto BUILD_REPORT_VERSION{ 1 };

		using Insertables = std::vector<std::pair<Descriptor, std::shared_ptr<Insertable>>>;
		using DependencyVector = std::vector<std::pair<Descriptor, std::pair<std::unordered_set<ResourceDependency>,
			std::unordered_set<ConfigurationDependency>>>>;
	
		static Insertables buildOrderToInsertables(const Configuration& config);
		static std::shared_ptr<Insertable> descriptorToInsertable(const Descriptor& descriptor, const Configuration& config);

		static json createBuildReport(const Configuration& config, const json& dependency_report);
		static void writeBuildReport(const fs::path& project_root, const json& j);

		static void cacheGlobules(const fs::path& project_root);

		static void moveTempToOutput(const Configuration& config);

	public:

		virtual void build(const Configuration& config) = 0;
	};
}
