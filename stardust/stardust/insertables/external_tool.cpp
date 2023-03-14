#include "external_tool.h"

namespace stardust {
	ExternalTool::ExternalTool(const std::string& tool_name, const fs::path& tool_exe_path, const std::string& tool_options,
		const fs::path& working_directory, const std::optional<fs::path>& temporary_rom, bool take_user_input, 
		const std::vector<fs::path>& static_dependencies, const std::optional<fs::path>& dependency_report_file_path)
		: tool_name(tool_name), tool_exe_path(tool_exe_path), tool_options(tool_options), working_directory(working_directory),
		take_user_input(take_user_input), temporary_rom(temporary_rom), static_dependencies(static_dependencies), 
		dependency_report_file_path(dependency_report_file_path)
	{
		if (!fs::exists(tool_exe_path)) {
			throw ToolNotFoundException(fmt::format(
				"{} executable not found at {}",
				tool_name,
				tool_exe_path.string()
			));
		}

		if (!fs::exists(working_directory)) {
			throw NotFoundException(fmt::format(
				"Working directory {} not found for {}",
				working_directory.string(),
				tool_name
			));
		}

		if (temporary_rom.has_value() && !fs::exists(temporary_rom.value())) {
			throw RomNotFoundException(fmt::format(
				"Temporary ROM not found at {}",
				temporary_rom.value().string()
			));
		}
	}

	std::unordered_set<Dependency> ExternalTool::determineDependencies() {
		if (!dependency_report_file_path.has_value()) {
			throw DependencyException(fmt::format("No dependency report file specified for {}", tool_name));
		}

		if (!fs::exists(dependency_report_file_path.value())) {
			throw DependencyException(fmt::format(
				"No dependency report file found at {}, are you sure this is the "
				"correct path and that you have installed quickbuild correctly?",
				dependency_report_file_path.value().string()
			));
		}

		std::unordered_set<Dependency> dependencies{};
		std::transform(static_dependencies.begin(), static_dependencies.end(), std::inserter(dependencies, dependencies.begin()),
			[](const auto& entry) { return Dependency(entry); });
		const auto reported{ Insertable::extractDependenciesFromReport(dependency_report_file_path.value()) };

		dependencies.insert(reported.begin(), reported.end());
		dependencies.insert(tool_exe_path);

		return dependencies;
	}

	void ExternalTool::insert() {
		// delete potential previous dependency report
		if (dependency_report_file_path.has_value()) {
			fs::remove(dependency_report_file_path.value());
		}

		spdlog::info(fmt::format("Running {}", tool_name));
		spdlog::debug(fmt::format(
			"Running {} using {} and options {}",
			tool_name,
			tool_exe_path.string(),
			tool_options
		));

		const auto prev_folder{ fs::current_path() };
		fs::current_path(working_directory);

		int exit_code;

		if (take_user_input) {
			exit_code = bp::system(fmt::format(
				"\"{}\" {}{}",
				tool_exe_path.string(),
				tool_options,
				temporary_rom.has_value() ? " \"" + temporary_rom.value().string() + '"' : ""
			), bp::std_in < stdin, bp::std_out > stdout, bp::std_err > stderr);
		}
		else {
			exit_code = bp::system(fmt::format(
				"\"{}\" {}{}",
				tool_exe_path.string(),
				tool_options,
				temporary_rom.has_value() ? " \"" + temporary_rom.value().string() + '"' : ""
			), bp::std_in < bp::null, bp::std_out > stdout, bp::std_err > stderr);
		}
		fs::current_path(prev_folder);

		if (exit_code == 0) {
			spdlog::info(fmt::format("Successfully ran {}!", tool_name));
		}
		else {
			throw InsertionException(fmt::format(
				"Running {} failed",
				tool_name
			));
		}
	}
}
