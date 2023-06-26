#include "emulators.h"

namespace callisto {
	Emulators::Emulators(const Configuration& config) {
		emulators.insert(config.emulator_configurations.begin(), config.emulator_configurations.end());
	}

	std::vector<std::string> Emulators::getEmulatorNames() const {
		std::vector<std::string> names{};

		for (const auto& [name, _] : emulators) {
			names.push_back(name);
		}

		return names;
	}

	void Emulators::launch(const std::string& emulator_name, const fs::path& rom_path) {
		if (emulators.find(emulator_name) == emulators.end()) {
			throw CallistoException(fmt::format("Emulator {} not found", emulator_name));
		}

		const auto emulator{ emulators.at(emulator_name) };
		const auto emulator_path{ emulator.executable.getOrThrow().string() };

		if (!fs::exists(emulator_path)) {
			throw NotFoundException(fmt::format("Emulator {} not found at {}", emulator_name, emulator_path));
		}

		bp::spawn(fmt::format(
			"\"{}\" {} \"{}\"",
			emulator_path,
			emulator.options.getOrDefault(""),
			rom_path.string()
		));
	}
}
