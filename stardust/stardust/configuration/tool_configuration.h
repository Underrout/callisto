#pragma once

#include <toml.hpp>

#include "config_variable.h"
#include "config_exception.h"

namespace stardust {
	class ToolConfiguration {
	public:
		PathConfigVariable working_directory;
		PathConfigVariable executable;
		StringConfigVariable options;

		StaticResourceDependencyConfigVariable static_dependencies;
		PathConfigVariable dependency_report_file;

		BoolConfigVariable pass_rom;
		BoolConfigVariable takes_user_input;

		ToolConfiguration(const std::string& tool_name) :
			working_directory(PathConfigVariable({ "tools", "generic", tool_name, "directory" })),
			executable(PathConfigVariable({ "tools", "generic", tool_name, "executable" })),
			takes_user_input(BoolConfigVariable({"tools", tool_name, "takes_user_input"})),
			options(StringConfigVariable({ "tools", "generic", tool_name, "options" })),
			static_dependencies(StaticResourceDependencyConfigVariable({ "tools", "generic", tool_name, "static_dependencies" })),
			dependency_report_file(PathConfigVariable({ "tools", "generic", tool_name, "dependency_report_file" })),
			pass_rom(BoolConfigVariable({"tools", "generic", tool_name, "pass_rom"})) 
		{}
	};
}
