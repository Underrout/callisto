#pragma once

#include <toml.hpp>

#include "config_variable.h"
#include "config_exception.h"

namespace stardust {
	class EmulatorConfiguration {
	public:
		PathConfigVariable executable;
		StringConfigVariable options;

		EmulatorConfiguration(const std::string& emulator_name) : 
			executable(PathConfigVariable({ "emulators", emulator_name, "executable" })),
			options(StringConfigVariable({ "emulators", emulator_name, "options" })) {}
	};
}
