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

		ExtendablePathVectorConfigVariable static_dependencies;
		PathConfigVariable dependency_report_file;

		BoolConfigVariable dont_pass_rom;

		ToolConfiguration(const std::string& tool_name) :
			working_directory(PathConfigVariable({ "tools", "generic", tool_name, "directory" })),
			executable(PathConfigVariable({ "tools", "generic", tool_name, "executable" })),
			options(StringConfigVariable({ "tools", "generic", tool_name, "options" })),
			static_dependencies(ExtendablePathVectorConfigVariable({ "tools", "generic", tool_name, "static_dependencies" })),
			dependency_report_file(PathConfigVariable({ "tools", "generic", tool_name, "dependency_report_file" })),
			dont_pass_rom(BoolConfigVariable({"tools", "generic", tool_name, "dont_pass_rom"})) 
		{}
	};
}
