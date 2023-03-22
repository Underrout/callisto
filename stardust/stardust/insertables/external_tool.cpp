#include "external_tool.h"

namespace stardust {
	ExternalTool::ExternalTool(const std::string& name, const Configuration& config, const ToolConfiguration& tool_config)
		: tool_name(name), tool_exe_path(tool_config.executable.getOrThrow()),
		tool_options(tool_config.options.getOrDefault("")),
		working_directory(tool_config.working_directory.getOrThrow()),
		take_user_input(tool_config.takes_user_input.getOrDefault(false)),
		pass_rom(tool_config.pass_rom.getOrDefault(true)),
		temporary_rom(config.temporary_rom.getOrThrow()), 
		static_dependencies(tool_config.static_dependencies.getOrDefault({})),
		dependency_report_file_path(tool_config.dependency_report_file.getOrDefault({}))
	{
		registerConfigurationDependency(tool_config.executable);
		registerConfigurationDependency(tool_config.options, ConfigurationDependency::Policy::REINSERT);
		registerConfigurationDependency(tool_config.working_directory, ConfigurationDependency::Policy::REINSERT);
		registerConfigurationDependency(tool_config.pass_rom, ConfigurationDependency::Policy::REINSERT);

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

	std::unordered_set<ResourceDependency> ExternalTool::determineDependencies() {
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

		std::unordered_set<ResourceDependency> dependencies{};
		std::transform(static_dependencies.begin(), static_dependencies.end(), std::inserter(dependencies, dependencies.begin()),
			[](const auto& entry) { return ResourceDependency(entry); });
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
				pass_rom ? " \"" + temporary_rom.value().string() + '"' : ""
			), bp::std_in < stdin, bp::std_out > stdout, bp::std_err > stderr);
		}
		else {
			exit_code = bp::system(fmt::format(
				"\"{}\" {}{}",
				tool_exe_path.string(),
				tool_options,
				pass_rom ? " \"" + temporary_rom.value().string() + '"' : ""
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
