#pragma once

#include <spdlog/spdlog.h>

#include "builder.h"
#include "../configuration/configuration.h"
#include "../insertables/initial_patch.h"
#include "must_rebuild_exception.h"
#include "../insertables/module.h"
#include "../saver/saver.h"

namespace callisto {
	class QuickBuilder : public Builder {
	public:
		enum class Result {
			SUCCESS,
			NO_WORK
		};
	protected:
		static constexpr auto MAX_ROM_SIZE = 16 * 1024 * 1024;

		json report;

		void checkBuildReportFormat() const;
		void checkBuildOrderChange(const Configuration& config) const;
		static void checkProblematicLevelChanges(const fs::path& levels_path, const std::unordered_set<int>& old_level_numbers);
		void checkRebuildConfigDependencies(const json& dependencies, const Configuration& config) const;
		void checkRebuildResourceDependencies(const json& dependencies, const fs::path& project_root, size_t starting_index = 0) const;
		std::optional<ConfigurationDependency> checkReinsertConfigDependencies(const json& config_dependencies, const Configuration& config) const;
		std::optional<ResourceDependency> checkReinsertResourceDependencies(const json& resource_dependencies) const;

		static void cleanModule(const fs::path& module_source_path, const fs::path& temporary_rom_path, const fs::path& project_root);
		static void copyOldModuleOutput(const std::vector<fs::path>& module_output_paths, const fs::path& project_root);

		static bool hijacksGoneBad(const std::vector<std::pair<size_t, size_t>>& old_hijacks, 
			const std::vector<std::pair<size_t, size_t>>& new_hijacks);

	public:
		Result build(const Configuration& config);

		QuickBuilder(const fs::path& project_root);
	};
}
