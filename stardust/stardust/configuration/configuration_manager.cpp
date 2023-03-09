#include "configuration_manager.h"

namespace stardust {
	ConfigurationManager::ConfigurationManager(const fs::path& stardust_directory) : 
		stardust_root(stardust_directory) {

		// ensure our settings folder exists
		fs::create_directories(fs::path(sago::getConfigHome()) / USER_SETTINGS_FOLDER_NAME);
	}

	std::vector<std::string> ConfigurationManager::getProfileNames() const {
		const auto profiles_folder{ stardust_root / PROFILE_FOLDER_NAME };

		std::vector<std::string> profile_names{};

		for (const auto& entry : fs::directory_iterator(profiles_folder)) {
			if (fs::is_directory(entry)) {
				profile_names.push_back(entry.path().stem().string());
			}
		}

		return profile_names;
	}

	Configuration ConfigurationManager::getConfiguration(std::optional<std::string> current_profile) const {
		Configuration::ConfigFileMap config_file_map{};
		Configuration::VariableFileMap variable_file_map{};

		std::vector<fs::path> config_root_folders{
			fs::path(sago::getConfigHome()) / USER_SETTINGS_FOLDER_NAME,
			stardust_root
		};

		if (current_profile.has_value()) {
			config_root_folders.push_back(stardust_root / PROFILE_FOLDER_NAME / current_profile.value());
		}

		for (int i{ 0 }; i != config_root_folders.size(); ++i) {
			const auto& config_root_folder{ config_root_folders.at(i) };
			const auto config_level{ static_cast<ConfigurationLevel>(i) };

			std::vector<fs::path> config_files{};

			for (const auto& entry : fs::directory_iterator(config_root_folder)) {
				if (fs::is_regular_file(entry) && entry.path().extension() == ".toml" && entry.path().filename() != VARIABLE_FILE_NAME) {
					config_files.push_back(entry.path());
				}
			}
			
			config_file_map[config_level] = config_files;

			const auto potential_variable_file{ config_root_folder / VARIABLE_FILE_NAME };
			if (fs::exists(potential_variable_file)) {
				variable_file_map[config_level] = potential_variable_file;
			}
			else {
				if (config_files.size() == 1) {
					variable_file_map[config_level] = config_files.at(0);
				}
			}
		}

		return Configuration(config_file_map, variable_file_map, stardust_root);
	}
}
