#include "binary_map16.h"

namespace callisto {
	BinaryMap16::BinaryMap16(const Configuration& config)
		: LunarMagicInsertable(config), 
		map16_file_path(registerConfigurationDependency(config.map16, Policy::REINSERT).getOrThrow())
	{
		registerConfigurationDependency(config.use_text_map16_format, Policy::REINSERT);
	}

	std::unordered_set<ResourceDependency> BinaryMap16::determineDependencies() {
		auto dependencies{ LunarMagicInsertable::determineDependencies() };
		dependencies.insert(map16_file_path);
		return dependencies;
	}

	void BinaryMap16::insert() {
		checkLunarMagicExists();

		if (!fs::exists(map16_file_path)) {
			throw ResourceNotFoundException(fmt::format(
				colors::EXCEPTION,
				"Binary Map16 file not found at {}",
				map16_file_path.string()
			));
		}

		spdlog::info(fmt::format(colors::RESOURCE, "Inserting Map16"));
		spdlog::debug(fmt::format(
			"Inserting binary map16 file at {} into temporary ROM at {}",
			map16_file_path.string(),
			temporary_rom_path.string()
		));

		const auto exit_code{ callLunarMagic(
			"-ImportAllMap16", 
			temporary_rom_path.string(), 
			map16_file_path.string())
		};

		if (exit_code == 0) {
			spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Successfully inserted Map16!"));
		}
		else {
			throw InsertionException(fmt::format(
				colors::EXCEPTION,
				"Failed to insert binary Map16 file {} into temporary ROM {}",
				map16_file_path.string(),
				temporary_rom_path.string()
			));
		}
	}
}
