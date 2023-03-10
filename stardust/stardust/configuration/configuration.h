#pragma once

#include <filesystem>
#include <array>
#include <map>

#include <boost/range.hpp>
#include <toml.hpp>
#include <fmt/core.h>

#include "tool_configuration.h"
#include "emulator_configuration.h"
#include "config_variable.h"
#include "config_exception.h"

namespace fs = std::filesystem;

namespace stardust {
	class Configuration {
	public:
		using ConfigFileMap = std::map<ConfigurationLevel, std::vector<fs::path>>;
		using VariableFileMap = std::map<ConfigurationLevel, fs::path>;

	private:
		static constexpr std::array VALID_STATIC_BUILD_ORDER_SYMBOLS{
			"Globules", "Graphics", "ExGraphics", "Map16", "TitleMoves", "SharedPalettes",
			"Overworld", "Titlescreen", "Credits", "GlobalExAnimation", 
			"Patches", "PIXI", "Levels", "AddMusicK"
		};

		using ParsedConfigFileMap = std::map<ConfigurationLevel, std::vector<toml::value>>;
		using UserVariableMap = std::map<ConfigurationLevel, std::map<std::string, std::string>>;

		static UserVariableMap parseVariableFiles(const VariableFileMap& variable_file_map);
		static ParsedConfigFileMap parseConfigFiles(const ConfigFileMap& config_file_map);

		void discoverProjectRoot(const ParsedConfigFileMap& config_file_map, const UserVariableMap& user_variable_map,
			const fs::path& stardust_root_directory);
		void discoverLogFilePath(const ParsedConfigFileMap& config_file_map, const UserVariableMap& user_variable_map);
		void discoverGenericTools(const ParsedConfigFileMap& config_file_map, const UserVariableMap& user_variable_map);
		void discoverToolDirectories(const ParsedConfigFileMap& config_file_map, const UserVariableMap& user_variable_map);
		void discoverEmulators(const ParsedConfigFileMap& config_file_map, const UserVariableMap& user_variable_map);
		
		void fillInConfiguration(const ParsedConfigFileMap& config_file_map, const UserVariableMap& user_variable_map);
		void fillOneConfigurationLevel(const std::vector<toml::value>& config_files, ConfigurationLevel level, 
			const std::map<std::string, std::string>& user_variables);
		void fillOneConfigurationFile(const toml::value& config_file, ConfigurationLevel level, 
			const std::map<std::string, std::string>& user_variables);
		void discoverBuildOrder(const toml::value& config_file, ConfigurationLevel level,
			const std::map<std::string, std::string>& user_variables);
		bool verifyBuildOrder(const toml::array& build_order, const std::map<std::string, std::string>& user_variables) const;
		bool isValidStaticBuildOrderSymbol(const std::string& symbol) const;
		bool isValidPatchSymbol(const fs::path& patch_path) const;
		bool isValidGlobuleSymbol(const fs::path& globule_path) const;

	public:
		static std::map<std::string, std::string> parseUserVariables(const toml::value& table,
			const std::map<std::string, std::string>& previous_user_variables = {});

		Configuration(const ConfigFileMap& config_file_map, const VariableFileMap& variable_file_map, 
			const fs::path& stardust_root_directory);

		StringConfigVariable config_name{ {"settings", "config_name"} };
		PathConfigVariable project_root{ {"settings", "project_root"} };
		IntegerConfigVariable rom_size{ {"settings", "rom_size"} };
		PathConfigVariable log_file{ {"settings", "log_file"} };

		PathConfigVariable project_rom{ {"output", "project_rom"} };
		PathConfigVariable temporary_rom{ {"output", "temporary_rom"} };
		PathConfigVariable bps_package{ {"output", "bps_package"} };

		PathConfigVariable flips_path{ {"tools", "FLIPS", "executable"} };

		PathConfigVariable lunar_magic_path{ {"tools", "LunarMagic", "executable"} };
		StringConfigVariable lunar_magic_level_import_flags{ {"tools", "LunarMagic", "level_import_flags"} };

		PathConfigVariable pixi_working_dir{ {"tools", "PIXI", "directory"} };
		PathConfigVariable pixi_list_file{ {"tools", "PIXI", "list_file"} };
		StringConfigVariable pixi_options{ {"tools", "PIXI", "options"} };
		ExtendablePathVectorConfigVariable pixi_static_dependencies{ {"tools", "PIXI", "static_dependencies"} };
		PathConfigVariable pixi_dependency_report_file{ {"tools", "PIXI", "dependency_report_file"} };

		PathConfigVariable amk_working_dir{ {"tools", "AddMusicK", "directory"} };
		PathConfigVariable amk_path{ {"tools", "AddMusicK", "executable"} };
		StringConfigVariable amk_options{ {"tools", "AddMusicK", "options"} };
		ExtendablePathVectorConfigVariable amk_static_dependencies{ {"tools", "AddMusicK", "static_dependencies"} };
		PathConfigVariable amk_dependency_report_file{ {"tools", "AddMusicK", "dependency_report_file"} };

		PathConfigVariable initial_patch{ {"resources", "initial_patch"} };
		PathConfigVariable levels{ {"resources", "levels"} };
		PathConfigVariable shared_palettes{ {"resources", "shared_palettes"} };
		PathConfigVariable map16{ {"resources", "map16"} };
		PathConfigVariable overworld{ {"resources", "overworld"} };
		PathConfigVariable titlescreen{ {"resources", "titlescreen"} };
		PathConfigVariable credits{ {"resources", "credits"} };
		PathConfigVariable global_exanimation{ {"resources", "global_exanimation"} };

		ExtendablePathVectorConfigVariable patches{ {"resources", "patches"} };
		ExtendablePathVectorConfigVariable globules{ {"resources", "globules"} };

		PathConfigVariable globule_header{ {"resources", "globule_header"} };

		StringVectorConfigVariable build_order{ {"orders", "build_order"} };

		std::map<std::string, ToolConfiguration> generic_tool_configurations{};

		std::map<std::string, EmulatorConfiguration> emulator_configurations{};
	};
}
