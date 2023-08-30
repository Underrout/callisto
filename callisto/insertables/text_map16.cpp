#include "text_map16.h"

namespace callisto {
	TextMap16::TextMap16(const Configuration& config)
		: LunarMagicInsertable(config), map16_folder_path(config.map16.getOrThrow())
	{
		registerConfigurationDependency(config.map16, Policy::REINSERT);
		registerConfigurationDependency(config.use_text_map16_format, Policy::REINSERT);
	}

	fs::path TextMap16::getTemporaryMap16FilePath() const {
		return temporary_rom_path.parent_path() / (map16_folder_path.string() + ".map16");
	}

	fs::path TextMap16::generateTemporaryMap16File() const
	{
		spdlog::info(fmt::format(colors::CALLISTO, "Generating Map16 file from text files"));
		spdlog::debug(fmt::format(
			"Generating temporary map16 file from map16 folder {}",
			map16_folder_path.string(),
			temporary_rom_path.string()
		));

		const auto temp_map16{ getTemporaryMap16FilePath() };
		const auto current_path{ fs::current_path() };

		try {
			HumanReadableMap16::to_map16::convert(map16_folder_path, temp_map16);
		}
		catch (HumanMap16Exception& e) {
			fs::current_path(current_path);
			throw InsertionException(fmt::format(colors::EXCEPTION, "Failed to convert map16 folder to file with following exception:\n\r{}",
				e.get_detailed_error_message()));
		}

		fs::current_path(current_path);
		return temp_map16;
	}

	void TextMap16::deleteTemporaryMap16File() const
	{
		try {
			fs::remove(getTemporaryMap16FilePath());
		}
		catch (const fs::filesystem_error&) {
			spdlog::warn(fmt::format(
				colors::WARNING,
				"WARNING: Failed to delete temporary Map16 file {}",
				getTemporaryMap16FilePath().string()
			));
		}
	}

	std::unordered_set<ResourceDependency> TextMap16::determineDependencies() {
		auto dependencies{ LunarMagicInsertable::determineDependencies() };
		const auto folder_dependencies{
			getResourceDependenciesFor(map16_folder_path, Policy::REINSERT)
		};
		dependencies.insert(folder_dependencies.begin(), folder_dependencies.end());
		return dependencies;
	}

	void TextMap16::insert() {
		if (!fs::exists(map16_folder_path)) {
			throw ResourceNotFoundException(fmt::format(
				colors::EXCEPTION,
				"Map16 folder not found at {}",
				map16_folder_path.string()
			));
		}

		spdlog::info(fmt::format(colors::RESOURCE, "Inserting Map16"));
		spdlog::debug(fmt::format(
			"Generating and inserting map16 into temporary ROM at {}",
			map16_folder_path.string(),
			temporary_rom_path.string()
		));

		const auto temp_map16{ generateTemporaryMap16File() };

		const auto exit_code{ callLunarMagic(
			"-ImportAllMap16",
			temporary_rom_path.string(),
			temp_map16.string())
		};

		deleteTemporaryMap16File();

		if (exit_code == 0) {
			spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Successfully inserted Map16!"));
		}
		else {
			throw InsertionException(fmt::format(
				colors::EXCEPTION,
				"Failed to insert generated Map16 file {} into temporary ROM {}",
				temp_map16.string(),
				temporary_rom_path.string()
			));
		}
	}
}
