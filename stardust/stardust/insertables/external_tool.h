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

namespace fs = std::filesystem;
namespace bp = boost::process;

namespace stardust {
	class ExternalTool : public Insertable {
	protected:
		const std::optional<fs::path> temporary_rom;
		const std::string tool_name;
		const fs::path tool_exe_path;
		const std::string tool_options;
		const fs::path working_directory;
		bool take_user_input;
		const std::unordered_set<Dependency> static_dependencies;
		const std::optional<fs::path> dependency_report_file_path;

		std::unordered_set<Dependency> determineDependencies() override;

	public:
		ExternalTool(const std::string& tool_name, const fs::path& tool_exe_path, const std::string& tool_options,
			const fs::path& working_directory, const std::optional<fs::path>& temporary_rom, bool take_user_input, 
			const std::unordered_set<Dependency>& static_dependencies = {},
			const std::optional<fs::path>& dependency_report_file_path = {});

		void insert() override;
	};
}
