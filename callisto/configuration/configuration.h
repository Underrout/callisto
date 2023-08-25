#pragma once

#include <filesystem>
#include <array>
#include <map>
#include <unordered_set>
#include <variant>

#include <boost/range.hpp>
#include <toml.hpp>
#include <fmt/core.h>
#include <tsl/ordered_map.h>

#include "tool_configuration.h"
#include "emulator_configuration.h"
#include "module_configuration.h"
#include "config_variable.h"
#include "config_exception.h"
#include "../path_util.h"
#include "../descriptor.h"
#include "color_configuration.h"
#include "../colors.h"
#include "../globals.h"

namespace fs = std::filesystem;

namespace callisto {
	class Configuration {
	public:
		using ConfigFileMap = std::map<ConfigurationLevel, std::vector<fs::path>>;
		using VariableFileMap = std::map<ConfigurationLevel, fs::path>;
		using BuildOrder = std::vector<Descriptor>;
		using ConfigValueType = std::variant<std::monostate, std::string, bool, std::vector<fs::path>>;

	private:
		static constexpr std::array VALID_STATIC_BUILD_ORDER_SYMBOLS{
			"InitialPatch", "Modules", "Graphics", "ExGraphics", "Map16", "TitleScreenMovement", "SharedPalettes",
			"Overworld", "TitleScreen", "Credits", "GlobalExAnimation", 
			"Patches", "Levels"
		};

		size_t module_count{ 0 };
		std::map<ConfigurationLevel, bool> modules_seen{};

		std::map<std::string, ConfigValueType> key_val_map{};

		StringVectorConfigVariable _build_order{ {"orders", "build_order"} };

		using ParsedConfigFileMap = std::map<ConfigurationLevel, std::vector<toml::value>>;
		using UserVariableMap = std::map<ConfigurationLevel, std::map<std::string, std::string>>;

		static UserVariableMap parseVariableFiles(const VariableFileMap& variable_file_map);
		static ParsedConfigFileMap parseConfigFiles(const ConfigFileMap& config_file_map);

		void discoverProjectRoot(const ParsedConfigFileMap& config_file_map, const UserVariableMap& user_variable_map,
			const fs::path& callisto_root_directory);
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
		bool isValidModuleSymbol(const fs::path& module_path) const;
		void verifyPatchModuleExclusivity();
		void verifyModuleExclusivity();
		void verifyPatchUniqueness();
		void finalizeBuildOrder();
		void transferConfiguredColors();

		bool trySet(StringConfigVariable& variable, const toml::value& table, 
			ConfigurationLevel level, const std::map<std::string, std::string>& user_variable_map);
		bool trySet(PathConfigVariable& variable, const toml::value& table, ConfigurationLevel level, const PathConfigVariable& relative_to,
			const std::map<std::string, std::string>& user_variable_map);
		bool trySet(BoolConfigVariable& variable, const toml::value& table, ConfigurationLevel level);
		bool trySet(ExtendablePathVectorConfigVariable& variable, const toml::value& table, ConfigurationLevel level,
			const fs::path& relative_to, const std::map<std::string, std::string>& user_variables);

		std::unordered_set<fs::path> getExplicitModules() const;
		std::unordered_set<fs::path> getExplicitPatches() const;

		std::vector<Descriptor> symbolToDescriptor(const std::string &symbol) const;

	public:
		static std::map<std::string, std::string> parseUserVariables(const toml::value& table,
			const std::map<std::string, std::string>& previous_user_variables = {});

		Configuration(const ConfigFileMap& config_file_map, const VariableFileMap& variable_file_map,
			const fs::path& callisto_root_directory);

		ConfigValueType getByKey(const std::string& key) const;

		const bool allow_user_input;

		std::unordered_set<Descriptor> ignored_conflict_symbols{};

		StringConfigVariable config_name{ {"settings", "config_name"} };
		PathConfigVariable project_root{ {"settings", "project_root"} };
		StringConfigVariable level_import_flag{ {"settings", "level_import_flag"} };
		StringConfigVariable rom_size{ {"settings", "rom_size"} };
		BoolConfigVariable use_text_map16_format{ {"settings", "use_text_map16_format"} };
		PathConfigVariable log_file{ {"settings", "log_file"} };
		PathConfigVariable clean_rom{ {"settings", "clean_rom"} };
		StringConfigVariable check_conflicts{ {"settings", "check_conflicts"} };
		PathConfigVariable conflict_log_file { {"settings", "conflict_log_file"} };
		StringVectorConfigVariable ignored_conflict_symbol_strings{ {"settings", "ignored_conflict_symbols"} };

		PathConfigVariable output_rom{ {"output", "output_rom"} };
		PathConfigVariable temporary_folder{ {"output", "temporary_folder"} };
		PathConfigVariable bps_package{ {"output", "bps_package"} };

		PathConfigVariable flips_path{ {"tools", "FLIPS", "executable"} };

		PathConfigVariable lunar_magic_path{ {"tools", "LunarMagic", "executable"} };
		StringConfigVariable lunar_magic_level_import_flags{ {"tools", "LunarMagic", "level_import_flags"} };

		PathConfigVariable initial_patch{ {"resources", "initial_patch"} };
		PathConfigVariable graphics{ {"resources", "graphics"} };
		PathConfigVariable ex_graphics{ {"resources", "ex_graphics"} };
		PathConfigVariable levels{ {"resources", "levels"} };
		PathConfigVariable shared_palettes{ {"resources", "shared_palettes"} };
		PathConfigVariable map16{ {"resources", "map16"} };
		PathConfigVariable overworld{ {"resources", "overworld"} };
		PathConfigVariable titlescreen{ {"resources", "titlescreen"} };
		PathConfigVariable title_moves{ {"resources", "titlescreen_movement"} };
		PathConfigVariable credits{ {"resources", "credits"} };
		PathConfigVariable global_exanimation{ {"resources", "global_exanimation"} };

		ExtendablePathVectorConfigVariable patches{ {"resources", "patches"} };
		tsl::ordered_map<fs::path, ModuleConfiguration> module_configurations{};

		PathConfigVariable module_header{ {"resources", "module_header"} };

		BuildOrder build_order{};

		std::map<std::string, ToolConfiguration> generic_tool_configurations{};

		std::map<std::string, EmulatorConfiguration> emulator_configurations{};

		std::map<std::string, ColorConfiguration> color_configurations{};
	};
}
