#include "configuration_manager.h"

namespace callisto {
	std::unordered_map<ConfigurationLevel, std::vector<fs::path>> ConfigurationManager::getConfigFilesToParse(const std::optional<std::string>& profile) {
		std::unordered_map<ConfigurationLevel, std::vector<fs::path>> config_files{};

		const auto user_folder{ PathUtil::getUserSettingsPath() };

		for (const auto& entry : fs::directory_iterator(user_folder)) {
			if (entry.path().extension() == ".toml") {
				config_files[ConfigurationLevel::USER].push_back(entry.path());
			}
		}

		for (const auto& entry : fs::directory_iterator(callisto_root)) {
			if (entry.path().extension() == ".toml") {
				config_files[ConfigurationLevel::PROJECT].push_back(entry.path());
			}
		}

		if (profile.has_value()) {
			for (const auto& entry : fs::directory_iterator(callisto_root / PROFILE_FOLDER_NAME / profile.value())) {
				config_files[ConfigurationLevel::PROFILE].push_back(entry.path());
			}
		}

		return config_files;
	}

	ConfigurationManager::ConfigurationManager(const fs::path& callisto_directory) :
		callisto_root(callisto_directory) {

		// ensure our settings folder exists
		fs::create_directories(PathUtil::getUserSettingsPath());
	}

	std::vector<std::string> ConfigurationManager::getProfileNames() const {
		const auto profiles_folder{ callisto_root / PROFILE_FOLDER_NAME };

		if (!fs::exists(profiles_folder) || !fs::is_directory(profiles_folder)) {
			return {};
		}

		std::vector<std::string> profile_names{};

		for (const auto& entry : fs::directory_iterator(profiles_folder)) {
			if (fs::is_directory(entry)) {
				profile_names.push_back(entry.path().stem().string());
			}
		}

		return profile_names;
	}

	std::shared_ptr<Configuration> ConfigurationManager::getConfiguration(std::optional<std::string> current_profile, bool allow_user_input) const {
		Configuration::ConfigFileMap config_file_map{};
		Configuration::VariableFileMap variable_file_map{};

		std::vector<fs::path> config_root_folders{
			PathUtil::getUserSettingsPath(),
			callisto_root
		};

		if (current_profile.has_value()) {
			config_root_folders.push_back(callisto_root / PROFILE_FOLDER_NAME / current_profile.value());
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

		return std::make_shared<Configuration>(config_file_map, variable_file_map, callisto_root, allow_user_input);
	}
}
