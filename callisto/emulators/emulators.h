#pragma once

#include <filesystem>
#include <vector>

#include <boost/process.hpp>
#include <boost/process/spawn.hpp>
#include <fmt/format.h>

#include "../configuration/emulator_configuration.h"
#include "../configuration/configuration.h"

namespace fs = std::filesystem;
namespace bp = boost::process;

namespace callisto {
	class Emulators {
	protected:
		std::map<std::string, EmulatorConfiguration> emulators{};

	public:
		Emulators() {};
		Emulators(const Configuration& config);

		std::vector<std::string> getEmulatorNames() const;

		void launch(const std::string& emulator_name, const fs::path& rom_path);
	};
}
