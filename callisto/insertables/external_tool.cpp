#include "external_tool.h"

namespace callisto {
	ExternalTool::ExternalTool(const std::string& name, const Configuration& config, const ToolConfiguration& tool_config)
		: tool_name(name), tool_exe_path(tool_config.executable.getOrThrow()),
		callisto_folder_path(PathUtil::getCallistoDirectoryPath(config.project_root.getOrThrow())),
		tool_options(tool_config.options.getOrDefault("")),
		working_directory(tool_config.working_directory.getOrThrow()),
		take_user_input(tool_config.takes_user_input.getOrDefault(false)),
		pass_rom(tool_config.pass_rom.getOrDefault(true)),
		temporary_rom(PathUtil::getTemporaryRomPath(config.temporary_folder.getOrThrow(), config.output_rom.getOrThrow())),
		static_dependencies(tool_config.static_dependencies.getOrDefault({})),
		dependency_report_file_path(tool_config.dependency_report_file.getOrDefault({}))
	{
		registerConfigurationDependency(tool_config.executable);
		registerConfigurationDependency(tool_config.options, Policy::REINSERT);
		registerConfigurationDependency(tool_config.working_directory, Policy::REINSERT);
		registerConfigurationDependency(tool_config.pass_rom, Policy::REINSERT);
	}

	std::unordered_set<ResourceDependency> ExternalTool::determineDependencies() {
		if (!dependency_report_file_path.has_value()) {
			throw DependencyException(fmt::format(colors::NOTIFICATION, "No dependency report file specified for {}", tool_name));
		}

		if (!fs::exists(dependency_report_file_path.value())) {
			throw NoDependencyReportFound(fmt::format(
				colors::NOTIFICATION,
				"No dependency report file found at {}",
				dependency_report_file_path.value().string()
			));
		}

		std::unordered_set<ResourceDependency> dependencies{};
		// little gross, but I need to do it like this because the files may have changed since when we
		// created the initial dependency objects, should probably not use objects at first and just 
		// do paths, but this is just how it is for now, I guess
		for (const auto& static_dependency : static_dependencies) {
			dependencies.insert(ResourceDependency(static_dependency.dependent_path, static_dependency.policy));
		}

		const auto reported{ Insertable::extractDependenciesFromReport(dependency_report_file_path.value()) };

		dependencies.insert(reported.begin(), reported.end());

		return dependencies;
	}

	void ExternalTool::createLocalCallistoFile() {
		const auto callisto_path{ PathUtil::convertToPosixPath(callisto_folder_path) };
		auto local_callisto_file_path{ working_directory };
		local_callisto_file_path /= ".callisto";

		std::ofstream out{ local_callisto_file_path.string()};
		out << callisto_path.string();
	}

	void ExternalTool::deleteLocalCallistoFile() {
		auto local_callisto_file_path{ working_directory };
		local_callisto_file_path /= ".callisto";

		fs::remove(local_callisto_file_path);
	}

	void ExternalTool::insert() {
		if (!fs::exists(temporary_rom)) {
			throw RomNotFoundException(fmt::format(
				colors::EXCEPTION,
				"Temporary ROM not found at {}",
				temporary_rom.string()
			));
		}

		if (!fs::exists(tool_exe_path)) {
			throw ToolNotFoundException(fmt::format(
				colors::EXCEPTION,
				"{} executable not found at {}",
				tool_name,
				tool_exe_path.string()
			));
		}

		if (!fs::exists(working_directory)) {
			throw NotFoundException(fmt::format(
				colors::EXCEPTION,
				"Working directory {} not found for {}",
				working_directory.string(),
				tool_name
			));
		}

		// delete potential previous dependency report
		if (dependency_report_file_path.has_value()) {
			fs::remove(dependency_report_file_path.value());
		}

		spdlog::info(fmt::format(colors::RESOURCE, "Running {}", tool_name));
		spdlog::debug(fmt::format(
			"Running {} using {} and options {}",
			tool_name,
			tool_exe_path.string(),
			tool_options
		));

		const auto prev_folder{ fs::current_path() };
		fs::current_path(working_directory);
		deleteLocalCallistoFile();
		createLocalCallistoFile();

		int exit_code;
		if (take_user_input) {
			exit_code = bp::system(fmt::format(
				"\"{}\" {}{}",
				tool_exe_path.string(),
				tool_options,
				pass_rom ? " \"" + temporary_rom.string() + '"' : ""
			), bp::std_in < stdin, bp::std_out > stdout, bp::std_err > stderr);
		}
		else {
			exit_code = bp::system(fmt::format(
				"\"{}\" {}{}",
				tool_exe_path.string(),
				tool_options,
				pass_rom ? " \"" + temporary_rom.string() + '"' : ""
			), bp::std_in < bp::null, bp::std_out > stdout, bp::std_err > stderr);
		}

		deleteLocalCallistoFile();
		fs::current_path(prev_folder);

		if (exit_code == 0) {
			spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Successfully ran {}!", tool_name));
		}
		else {
			throw InsertionException(fmt::format(
				colors::EXCEPTION,
				"Running {} failed",
				tool_name
			));
		}
	}
}
