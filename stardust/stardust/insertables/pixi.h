#pragma once

#include <filesystem>

#include <boost/program_options/parsers.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <pixi_api.h>

#include "rom_insertable.h"
#include "../insertion_exception.h"
#include "../dependency/dependency_exception.h"

#include "../configuration/configuration.h"
#include "../dependency/policy.h"

namespace fs = std::filesystem;

namespace stardust {
	class Pixi : public RomInsertable {
	protected:
		const fs::path pixi_folder_path;
		const std::string pixi_options;
		const std::vector<ResourceDependency> static_dependencies;
		const std::optional<fs::path> dependency_report_file_path;
		
		std::unordered_set<ResourceDependency> determineDependencies() override;

	public:
		Pixi(const Configuration& config);

		void insert() override;
	};
}
