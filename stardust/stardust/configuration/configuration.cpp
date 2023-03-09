#include "configuration.h"

namespace stardust {
	Configuration::Configuration(const ConfigFileMap& config_file_map, const VariableFileMap& variable_file_map, 
		const fs::path& stardust_root_directory) {

		const auto user_variables{ parseVariableFiles(variable_file_map) };
		const auto config_files{ parseConfigFiles(config_file_map) };

		discoverProjectRoot(config_files, user_variables, stardust_root_directory);
		discoverLogFilePath(config_files, user_variables);
		discoverGenericTools(config_files, user_variables);
		discoverToolDirectories(config_files, user_variables);
		discoverEmulators(config_files, user_variables);

		fillInConfiguration(config_files, user_variables);
	}

	std::map<std::string, std::string> Configuration::parseUserVariables(const toml::value& table,
		const std::map<std::string, std::string>& previous_user_variables) {

		std::map<std::string, std::string> user_variables{ };
		std::map<std::string, const toml::value&> toml_values{};

		std::map<std::string, std::string> local_user_variables{};
		std::map<std::string, const toml::value&> local_toml_values{};

		toml::table variables;

		try {
			variables = toml::find<toml::table>(table, "variables");
		}
		catch (const std::out_of_range&) {
			// no variable table in file
			return {};
		}

		for (const auto& [key, value] : variables) {
			if (user_variables.count(key) != 0) {
				throw TomlException2(
					value,
					toml_values.at(key),
					"Duplicate user variable",
					{
						"The same user variable cannot be specified multiple times at the same level of configuration"
					},
					fmt::format("User variable '{}' specified multiple times", key),
					"Previously specified here"
				);
			}

			auto joined_vars{ previous_user_variables };

			for (const auto& [key, value] : local_user_variables) {
				joined_vars[key] = value;
			}

			local_user_variables.insert({ 
				key, 
				ConfigVariable<toml::string, std::string>::formatUserVariables(value, joined_vars)
			});
			local_toml_values.insert({ key, value });
		}

		user_variables.insert(local_user_variables.begin(), local_user_variables.end());
		toml_values.insert(local_toml_values.begin(), local_toml_values.end());

		return user_variables;
	}

	Configuration::UserVariableMap Configuration::parseVariableFiles(const VariableFileMap& variable_file_map) {
		UserVariableMap user_variable_map{};

		for (int i{ 0 }; i != static_cast<int>(ConfigurationLevel::_COUNT); ++i) {
			const auto config_level{ static_cast<ConfigurationLevel>(i) };
			if (variable_file_map.count(config_level) != 0) {
				const auto potential_variable_file{ variable_file_map.at(config_level) };

				if (fs::exists(potential_variable_file) && fs::is_regular_file(potential_variable_file)) {
					auto inherited_variables{ config_level == ConfigurationLevel::PROFILE
						? user_variable_map.at(ConfigurationLevel::PROJECT) : std::map<std::string, std::string>() };

					user_variable_map.insert({
						config_level,
						parseUserVariables(toml::parse(potential_variable_file), inherited_variables)
						});
				}
				else {
					user_variable_map.insert({ config_level, {} });
				}
			}
			else {
				user_variable_map.insert({ config_level, {} });
			}
		}

		return user_variable_map;
	}

	Configuration::ParsedConfigFileMap Configuration::parseConfigFiles(const ConfigFileMap& config_file_map) {
		ParsedConfigFileMap parsed_config_file_map{};

		for (const auto& [level, config_file_paths] : config_file_map) {
			parsed_config_file_map.insert({ level, {} });
			for (const auto& config_file_path : config_file_paths) {
				parsed_config_file_map.at(level).push_back(toml::parse(config_file_path));
			}
		}

		for (int i = 0; i != static_cast<int>(ConfigurationLevel::_COUNT); ++i) {
			const auto config_level{ static_cast<ConfigurationLevel>(i) };
			if (parsed_config_file_map.count(config_level) == 0) {
				parsed_config_file_map.insert({ config_level, {} });
			}
		}

		return parsed_config_file_map;
	}

	void Configuration::discoverProjectRoot(const ParsedConfigFileMap& config_file_map, const UserVariableMap& user_variable_map,
		const fs::path& stardust_root_directory) {
		for (int i = 0; i != static_cast<int>(ConfigurationLevel::_COUNT); ++i) {
			const auto config_level{ static_cast<ConfigurationLevel>(i) };
			for (const auto& config : config_file_map.at(config_level)) {
				project_root.trySet(config, config_level, stardust_root_directory,
					user_variable_map.at(config_level));
			}
		}
	}

	void Configuration::discoverLogFilePath(const ParsedConfigFileMap& config_file_map, const UserVariableMap& user_variable_map) {
		for (int i = 0; i != static_cast<int>(ConfigurationLevel::_COUNT); ++i) {
			const auto config_level{ static_cast<ConfigurationLevel>(i) };
			for (const auto& config : config_file_map.at(config_level)) {
				log_file.trySet(config, config_level, project_root, user_variable_map.at(config_level));
			}
		}
	}

	void Configuration::discoverGenericTools(const ParsedConfigFileMap& config_file_map, const UserVariableMap& user_variable_map) {
		for (int i = 0; i != static_cast<int>(ConfigurationLevel::_COUNT); ++i) {
			const auto config_level{ static_cast<ConfigurationLevel>(i) };
			for (const auto& config : config_file_map.at(config_level)) {
				try {
					const auto& tools_table{ toml::find(config, "tools") };
					const auto& generic_tools_table{ toml::find<toml::table>(tools_table, "generic") };

					for (const auto& [key, value] : generic_tools_table) {
						generic_tool_configurations.insert({ key, ToolConfiguration(key) });
					}
				}
				catch (const std::out_of_range&) {
					continue;
				}
			}
		}
	}

	void Configuration::discoverToolDirectories(const ParsedConfigFileMap& config_file_map, const UserVariableMap& user_variable_map) {
		for (int i = 0; i != static_cast<int>(ConfigurationLevel::_COUNT); ++i) {
			const auto config_level{ static_cast<ConfigurationLevel>(i) };
			for (const auto& config : config_file_map.at(config_level)) {
				try {
					const auto& tools_table{ toml::find(config, "tools") };

					try {
						const auto& pixi_table{ toml::find(tools_table, "PIXI") };

						pixi_working_dir.trySet(config, config_level, project_root,
							user_variable_map.at(config_level));
					}
					catch (const std::out_of_range&) {
						// pass
					}

					try {
						const auto& pixi_table{ toml::find(tools_table, "AddMusicK") };

						amk_working_dir.trySet(config, config_level, project_root,
							user_variable_map.at(config_level));
					}
					catch (const std::out_of_range&) {
						// pass
					}

					const auto& generic_tools_table{ toml::find<toml::table>(tools_table, "generic") };

					for (const auto& [key, value] : generic_tools_table) {
						generic_tool_configurations.at(key).working_directory.trySet(
							config, config_level, project_root, user_variable_map.at(config_level)
						);
					}
				}
				catch (const std::out_of_range&) {
					continue;
				}
			}
		}
	}

	void Configuration::discoverEmulators(const ParsedConfigFileMap& config_file_map, const UserVariableMap& user_variable_map) {
		for (int i = 0; i != static_cast<int>(ConfigurationLevel::_COUNT); ++i) {
			const auto config_level{ static_cast<ConfigurationLevel>(i) };
			for (const auto& config : config_file_map.at(config_level)) {
				try {
					const auto& emulators_table{ toml::find<toml::table>(config, "emulators") };

					for (const auto& [key, value] : emulators_table) {
						EmulatorConfiguration emulator_config{ key };
						emulator_config.executable.trySet(config, config_level, project_root, user_variable_map.at(config_level));
						emulator_config.options.trySet(config, config_level, user_variable_map.at(config_level));
						emulator_configurations.insert({ key, emulator_config });
					}
				}
				catch (const std::out_of_range&) {
					continue;
				}
			}
		}
	}

	void Configuration::fillInConfiguration(const ParsedConfigFileMap& config_file_map, const UserVariableMap& user_variable_map) {
		for (int i = 0; i != static_cast<int>(ConfigurationLevel::_COUNT); ++i) {
			const auto config_level{ static_cast<ConfigurationLevel>(i) };
			fillOneConfigurationLevel(config_file_map.at(config_level), config_level, 
				user_variable_map.at(config_level));
		}
	}

	void Configuration::fillOneConfigurationLevel(const std::vector<toml::value>& config_files, ConfigurationLevel level,
		const std::map<std::string, std::string>& user_variables) {
		for (const auto& config_file : config_files) {
			fillOneConfigurationFile(config_file, level, user_variables);
		}
	}

	void Configuration::fillOneConfigurationFile(const toml::value& config_file, ConfigurationLevel level,
		const std::map<std::string, std::string>& user_variables) {

		const auto& root{ project_root };

		config_name.trySet(config_file, level, user_variables);

		rom_size.trySet(config_file, level);
		
		project_rom.trySet(config_file, level, root, user_variables);
		temporary_rom.trySet(config_file, level, root, user_variables);
		bps_package.trySet(config_file, level, root, user_variables);

		flips_path.trySet(config_file, level, root, user_variables);
		
		lunar_magic_path.trySet(config_file, level, root, user_variables);
		lunar_magic_level_import_flags.trySet(config_file, level, user_variables);

		pixi_list_file.trySet(config_file, level, pixi_working_dir, user_variables);
		pixi_options.trySet(config_file, level, user_variables);
		pixi_static_dependencies.trySet(config_file, level, pixi_working_dir, user_variables);
		pixi_dependency_report_file.trySet(config_file, level, pixi_working_dir, user_variables);

		amk_path.trySet(config_file, level, amk_working_dir, user_variables);
		amk_options.trySet(config_file, level, user_variables);
		amk_static_dependencies.trySet(config_file, level, amk_working_dir, user_variables);
		amk_dependency_report_file.trySet(config_file, level, amk_working_dir, user_variables);

		initial_patch.trySet(config_file, level, root, user_variables);
		levels.trySet(config_file, level, root, user_variables);
		shared_palettes.trySet(config_file, level, root, user_variables);
		map16.trySet(config_file, level, root, user_variables);
		overworld.trySet(config_file, level, root, user_variables);
		titlescreen.trySet(config_file, level, root, user_variables);
		credits.trySet(config_file, level, root, user_variables);
		global_exanimation.trySet(config_file, level, root, user_variables);

		patches.trySet(config_file, level, root, user_variables);
		globules.trySet(config_file, level, root, user_variables);

		globule_header.trySet(config_file, level, root, user_variables);

		build_order.trySet(config_file, level, user_variables);

		for (auto& [_, tool] : generic_tool_configurations) {
			tool.executable.trySet(config_file, level, tool.working_directory, user_variables);
			tool.options.trySet(config_file, level, user_variables);
			tool.static_dependencies.trySet(config_file, level, tool.working_directory, user_variables);
			tool.dependency_report_file.trySet(config_file, level, tool.working_directory, user_variables);
			tool.dont_pass_rom.trySet(config_file, level);
		}
	}
}
