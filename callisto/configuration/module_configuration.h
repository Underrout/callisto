#pragma once

#include <filesystem>

namespace fs = std::filesystem;

#include <toml.hpp>

#include "config_variable.h"
#include "config_exception.h"

namespace callisto {
	class ModuleConfiguration {
	public:
		PathConfigVariable input_path;
		ExtendablePathVectorConfigVariable real_output_paths;
		size_t index;

		ModuleConfiguration(size_t module_index)
			: index(module_index),
			input_path(PathConfigVariable({ "resources", "modules", module_index, "input_path" })),
			real_output_paths(ExtendablePathVectorConfigVariable({ "resources", "modules", module_index, "output_paths" }))
		{}
	};
}
