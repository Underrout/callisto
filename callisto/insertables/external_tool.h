#pragma once

#include <filesystem>

#include <boost/process/system.hpp>
#include <boost/process.hpp>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "../insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"
#include "../dependency/dependency_exception.h"
#include "../configuration/tool_configuration.h"
#include "../configuration/configuration.h"
#include "../dependency/policy.h"
#include "../dependency/resource_dependency.h"
#include "../path_util.h"

namespace fs = std::filesystem;
namespace bp = boost::process;

namespace callisto {
	class ExternalTool : public Insertable {
	protected:
		const fs::path temporary_rom;
		const std::string tool_name;
		const fs::path tool_exe_path;
		const std::string tool_options;
		const fs::path working_directory;
		bool take_user_input;
		bool pass_rom;
		const std::vector<ResourceDependency> static_dependencies;
		const std::optional<fs::path> dependency_report_file_path;
		const fs::path callisto_folder_path;

		std::unordered_set<ResourceDependency> determineDependencies() override;

		void createLocalCallistoFile();
		void deleteLocalCallistoFile();

	public:
		ExternalTool(const std::string& name, const Configuration& config, const ToolConfiguration& tool_config);

		void insert() override;
	};
}
