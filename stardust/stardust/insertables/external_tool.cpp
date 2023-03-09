#include "external_tool.h"

namespace stardust {
	ExternalTool::ExternalTool(const std::string& tool_name, const fs::path& tool_exe_path, const std::string& tool_options,
		const fs::path& working_directory, bool take_user_input)
		: tool_name(tool_name), tool_exe_path(tool_exe_path), tool_options(tool_options), working_directory(working_directory),
		take_user_input(take_user_input)
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
	}

	ExternalTool::ExternalTool(const std::string& tool_name, const fs::path& tool_exe_path, const std::string& tool_options)
		: ExternalTool(tool_name, tool_exe_path, tool_options, tool_exe_path.parent_path()) {}

	void ExternalTool::insert() {
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
				"\"{}\" {}",
				tool_exe_path.string(),
				tool_options
			), bp::std_in < stdin, bp::std_out > stdout, bp::std_err > stderr);
		}
		else {
			exit_code = bp::system(fmt::format(
				"\"{}\" {}",
				tool_exe_path.string(),
				tool_options
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
