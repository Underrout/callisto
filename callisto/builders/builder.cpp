#include "builder.h"

namespace callisto {
	Builder::Insertables Builder::buildOrderToInsertables(const Configuration& config) {
		Insertables insertables{};

		for (const auto& descriptor : config.build_order) {
			insertables.push_back({ descriptor, descriptorToInsertable(descriptor, config) });
		}

		return insertables;
	}

	std::shared_ptr<Insertable> Builder::descriptorToInsertable(const Descriptor& descriptor, const Configuration& config) {
		const auto symbol{ descriptor.symbol };
		const auto name{ descriptor.name };

		if (symbol == Symbol::INITIAL_PATCH) {
			return std::make_shared<InitialPatch>(config);
		}
		else if (symbol == Symbol::GRAPHICS) {
			return std::make_shared<Graphics>(config);
		}
		else if (symbol == Symbol::EX_GRAPHICS) {
			return std::make_shared<ExGraphics>(config);
		}
		else if (symbol == Symbol::MAP16) {
			if (config.use_text_map16_format.getOrDefault(false)) {
				return std::make_shared<TextMap16>(config);
			}
			else {
				return std::make_shared<BinaryMap16>(config);
			}
		}
		else if (symbol == Symbol::TITLE_SCREEN_MOVEMENT) {
			return std::make_shared<TitleMoves>(config);
		}
		else if (symbol == Symbol::SHARED_PALETTES) {
			return std::make_shared<SharedPalettes>(config);
		}
		else if (symbol == Symbol::OVERWORLD) {
			return std::make_shared<Overworld>(config);
		}
		else if (symbol == Symbol::TITLE_SCREEN) {
			return std::make_shared<TitleScreen>(config);
		}
		else if (symbol == Symbol::CREDITS) {
			return std::make_shared<Credits>(config);
		}
		else if (symbol == Symbol::GLOBAL_EX_ANIMATION) {
			return std::make_shared<GlobalExAnimation>(config);
		}
		else if (symbol == Symbol::LEVELS) {
			return std::make_shared<Levels>(config);
		}
		else if (symbol == Symbol::PATCH) {
			std::vector<fs::path> include_paths{ PathUtil::getCallistoDirectoryPath(config.project_root.getOrThrow()) };

			return std::make_shared<Patch>(
				config,
				name.value(),
				include_paths
			);
		} 
		else if (symbol == Symbol::MODULE) {
			const auto callisto_directory{ PathUtil::getCallistoDirectoryPath(config.project_root.getOrThrow()) };
			std::vector<fs::path> include_paths{ callisto_directory };

			return std::make_shared<Module>(
				config,
				name.value(),
				PathUtil::getCallistoAsmFilePath(config.project_root.getOrThrow()),
				module_addresses,
				include_paths
			);
		}
		else if (symbol == Symbol::EXTERNAL_TOOL) {
			const auto& tool_config{ config.generic_tool_configurations.at(name.value()) };
			return std::make_shared<ExternalTool>(
				name.value(),
				config,
				tool_config
			);
		}
		else {
			throw ConfigException(fmt::format(colors::EXCEPTION, "Unknown build order symbol"));
		}
	}

	json Builder::createBuildReport(const Configuration& config, const json& dependency_report) {
		spdlog::info(fmt::format(colors::CALLISTO, "Creating build report"));
		json report{};
		report["dependencies"] = dependency_report;
		report["file_format_version"] = BUILD_REPORT_VERSION;
		report["build_order"] = std::vector<std::string>();
		if (config.rom_size.isSet()) {
			report["rom_size"] = config.rom_size.getOrThrow();
		}
		else {
			report["rom_size"] = nullptr;
		}
		report["callisto_version"] = fmt::format("{}.{}.{}",
			CALLISTO_VERSION_MAJOR, CALLISTO_VERSION_MINOR, CALLISTO_VERSION_PATCH);

		for (const auto& descriptor : config.build_order) {
			report["build_order"].push_back(descriptor.toJson());
		}
		
		report["inserted_levels"] = std::vector<int>();
		if (config.levels.isSet()) {
			for (const auto& entry : fs::directory_iterator(config.levels.getOrThrow())) {
				if (entry.path().extension() != ".mwl") {
					continue;
				}
				try {
					const auto level_number{ Levels::getInternalLevelNumber(entry) };
					report["inserted_levels"].push_back(level_number.value());
				}
				catch (const std::exception& e) {
					throw InsertionException(fmt::format(
						colors::EXCEPTION,
						"Failed to determine source level number of level file '{}' with exception:\n\r{}",
						entry.path().string(), e.what()
					));
				}
			}
		}

		std::map<std::string, std::vector<std::string>> module_output_map{};
		for (const auto& [input_path, module_config] : config.module_configurations) {
			std::vector<std::string> module_outputs{};
			for (const auto& output_path : module_config.real_output_paths.getOrThrow()) {
				module_outputs.push_back(output_path.string());
			}
			module_output_map.insert({ input_path.string(), module_outputs });
		}
		report["module_outputs"] = module_output_map;

		return report;
	}

	void Builder::writeBuildReport(const fs::path& project_root, const json& j) {
		spdlog::info(fmt::format(colors::CALLISTO, "Writing build report"));
		std::ofstream build_report{ PathUtil::getBuildReportPath(project_root) };
		build_report << std::setw(4) << j << std::endl;
		build_report.close();
	}

	void Builder::cacheModules(const fs::path& project_root) {
		spdlog::info(fmt::format(colors::CALLISTO, "Caching modules"));
		const auto source{ PathUtil::getUserModuleDirectoryPath(project_root) };
		const auto target{ PathUtil::getModuleOldSymbolsDirectoryPath(project_root) };
		fs::create_directories(target);
		if (fs::exists(source)) {
			fs::remove_all(target);
			fs::copy(source, target, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
		}
	}

	void Builder::moveTempToOutput(const Configuration& config) {
		spdlog::info(fmt::format(colors::CALLISTO, "Moving temporary files to final output"));
		for (const auto& entry : fs::directory_iterator(config.temporary_folder.getOrThrow())) {
			if (fs::is_regular_file(entry)) {
				const auto file_name{ entry.path().stem().string() };
				const auto temporary_rom_name{ (PathUtil::getTemporaryRomPath(
					config.temporary_folder.getOrThrow(), config.output_rom.getOrThrow())).stem().string() };
				if (file_name.substr(0, temporary_rom_name.size()) == temporary_rom_name) {
					const auto source{ entry.path() };
					const auto target{ config.output_rom.getOrThrow().parent_path() / 
						(config.output_rom.getOrThrow().stem().string() + entry.path().extension().string())};

					while (true) {
						try {
							fs::copy_file(source, target, fs::copy_options::overwrite_existing);
							break;
						}
						catch (const std::runtime_error& e) {
							if (!config.allow_user_input) {
								throw e;
							}
							char input;
							do {
								spdlog::error(fmt::format(colors::EXCEPTION, 
									"Failed to copy '{}' to '{}', try again? Y/N", source.string(), target.string()));
								std::cin >> input;
								std::cin.ignore((std::numeric_limits<std::streamsize>::max)());
							} while (input != 'y' && input != 'Y' && input != 'n' && input != 'N');

							if (input == 'n' || input == 'N') {
								throw e;
							}
						}
					}

					try {
						fs::remove(source);
					}
					catch (const std::runtime_error&) {
						spdlog::warn(fmt::format(colors::WARNING, "Failed to remove temporary file '{}'", source.string()));
					}
				}
			}
		}
	}
	
	void Builder::init(const Configuration& config) {
		spdlog::info(fmt::format(colors::CALLISTO, "Initializing callisto directory"));
		spdlog::info("");
		ensureCacheStructure(config);
		generateCallistoAsmFile(config);
		fs::create_directories(config.temporary_folder.getOrThrow());
		fs::create_directories(config.output_rom.getOrThrow().parent_path());

		tryConvenienceSetup(config);

		if (config.levels.isSet() && fs::exists(config.levels.getOrThrow())) {
			spdlog::info(fmt::format(colors::CALLISTO, "Ensuring normalized level filenames"));
			spdlog::info("");
			Levels::normalizeMwls(config.levels.getOrThrow(), config.allow_user_input);
		}
	}

	void Builder::tryConvenienceSetup(const Configuration& config) {
		if (!config.allow_user_input || !config.clean_rom.isSet() || 
			!fs::exists(config.clean_rom.getOrThrow()) || fs::exists(config.output_rom.getOrThrow())) {
			return;
		}

		// Check if any extractables are already present
		bool at_least_one_present{ false };
		bool at_least_one_set{ false };
		for (const auto& var : {
			config.levels,
			config.graphics,
			config.ex_graphics,
			config.shared_palettes,
			config.map16,
			config.credits,
			config.global_exanimation,
			config.overworld,
			config.titlescreen
			}) {
			if (var.isSet()) {
				at_least_one_set = true;
				if (fs::exists(var.getOrThrow())) {
					at_least_one_present = true;
					break;
				}
			}
		}

		if (at_least_one_set && !at_least_one_present) {
			PromptUtil::yesNoPrompt(
				"Some resources which are configured to be inserted are not present, extract them from your clean ROM?",
				[&] {
					spdlog::info("Exporting missing resources from clean ROM");
					convenienceSetup(config); 
				}
			);
		}
	}

	void Builder::convenienceSetup(const Configuration& config) {
		const auto temp_rom_path{ PathUtil::getTemporaryRomPath(config.temporary_folder.getOrThrow(),
			config.output_rom.getOrThrow())};

		fs::copy(config.clean_rom.getOrThrow(), temp_rom_path, fs::copy_options::overwrite_existing);

		if (config.initial_patch.isSet()) {
			InitialPatch init_patch{ config };
			init_patch.insert(temp_rom_path);
		}

		for (const auto& var : { config.levels, config.ex_graphics }) {
			if (var.isSet()) {
				try {
					fs::create_directories(var.getOrThrow());
					spdlog::info("Created empty folder at '{}'", var.getOrThrow().string());
				}
				catch (const std::exception& e) {
					spdlog::error("Failed to create empty folder at '{}' with exception:\n\r{}", var.getOrThrow().string(), e.what());
				}
			}
		}

		if (config.graphics.isSet()) {
			createFolderStructureFor(config.graphics);
			extractables::Graphics graphics{ config, temp_rom_path };
			graphics.extract();
		}

		if (config.shared_palettes.isSet()) {
			createFolderStructureFor(config.shared_palettes);
			extractables::SharedPalettes shared_palettes{ config, temp_rom_path };
			shared_palettes.extract();
		}

		if (config.map16.isSet()) {
			createFolderStructureFor(config.map16);
			if (config.use_text_map16_format.getOrDefault(false)) {
				extractables::TextMap16 text_map16{ config, temp_rom_path };
				text_map16.extract();
			}
			else {
				extractables::BinaryMap16 binary_map16{ config, temp_rom_path };
			}
		}

		if (config.credits.isSet()) {
			createFolderStructureFor(config.credits);
			extractables::Credits credits{ config, temp_rom_path };
			credits.extract();
		}

		if (config.titlescreen.isSet()) {
			createFolderStructureFor(config.titlescreen);
			extractables::TitleScreen title_screen{ config, temp_rom_path };
			title_screen.extract();
		}

		if (config.overworld.isSet()) {
			createFolderStructureFor(config.overworld);
			extractables::Overworld overworld{ config, temp_rom_path };
			overworld.extract();
		}

		if (config.global_exanimation.isSet()) {
			createFolderStructureFor(config.global_exanimation);
			extractables::GlobalExAnimation global_exanimation{ config, temp_rom_path };
			global_exanimation.extract();
		}
	}

	void Builder::ensureCacheStructure(const Configuration& config) {
		const auto project_root{ config.project_root.getOrThrow() };

		fs::remove_all(PathUtil::getUserModuleDirectoryPath(project_root));

		fs::create_directories(PathUtil::getModuleCleanupDirectoryPath(project_root));
		fs::create_directories(PathUtil::getModuleOldSymbolsDirectoryPath(project_root));
		fs::create_directories(PathUtil::getUserModuleDirectoryPath(project_root));
	}

	void Builder::generateCallistoAsmFile(const Configuration& config) {
		const auto module_folder{ PathUtil::getUserModuleDirectoryPath(config.project_root.getOrThrow()) };

		const auto info_string{ fmt::format(
			"includeonce\n\n"
			"; Asar compatible file containing information about callisto, can be imported using incsrc as needed\n\n"
			"; Define containing the name of the active profile\n"
			"!{}_{} = \"{}\"\n\n"
			"; Marker define to determine that callisto is assembling a file\n"
			"!{}_{} = 1\n\n"
			"; Define containing callisto's version number as a string\n"
			"!{}_{} = \"{}.{}.{}\"\n\n"
			"; Defines containing callisto's version number as individual numbers\n"
			"!{}_{}_{} = {}\n"
			"!{}_{}_{} = {}\n"
			"!{}_{}_{} = {}\n\n"
			"; Define containing path to callisto's module imprint folder\n"
			"!{}_{} = \"{}\"\n\n"
			"macro call_module(module_label)\n"
			"\tPHB\n"
			"\tLDA.b #<module_label>>>16\n"
			"\tPHA\n"
			"\tPLB\n"
			"\tJSL <module_label>\n"
			"\tPLB\n"
			"endmacro\n\n"
			"macro include_module(module_name)\n"
			"\tincsrc \"!{}_{}/<module_name>\"\n"
			"endmacro\n",
			DEFINE_PREFIX, PROFILE_DEFINE_NAME, config.config_name.getOrThrow(),
			DEFINE_PREFIX, ASSEMBLING_DEFINE_NAME,
			DEFINE_PREFIX, VERSION_DEFINE_NAME, CALLISTO_VERSION_MAJOR, CALLISTO_VERSION_MINOR, CALLISTO_VERSION_PATCH,
			DEFINE_PREFIX, VERSION_DEFINE_NAME, MAJOR_VERSION_DEFINE_NAME, CALLISTO_VERSION_MAJOR,
			DEFINE_PREFIX, VERSION_DEFINE_NAME, MINOR_VERSION_DEFINE_NAME, CALLISTO_VERSION_MINOR,
			DEFINE_PREFIX, VERSION_DEFINE_NAME, PATCH_VERSION_DEFINE_NAME, CALLISTO_VERSION_PATCH,
			DEFINE_PREFIX, MODULE_FOLDER_PATH_DEFINE_NAME, PathUtil::convertToPosixPath(module_folder).string(),
			DEFINE_PREFIX, MODULE_FOLDER_PATH_DEFINE_NAME
		) };

		writeIfDifferent(info_string, PathUtil::getCallistoAsmFilePath(config.project_root.getOrThrow()));
	}

	void Builder::checkCleanRom(const fs::path& clean_rom_path) {
		if (!fs::exists(clean_rom_path)) {
			throw InsertionException(fmt::format(colors::EXCEPTION, "No clean ROM found at '{}'", clean_rom_path.string()));
		}

		if (clean_rom_path.extension() != ".smc") {
			spdlog::warn(fmt::format(colors::WARNING, "Your clean ROM at '{}' does not have a .smc extension", clean_rom_path.string()));
		}

		const auto rom_size{ fs::file_size(clean_rom_path) };		
		if (rom_size != CLEAN_ROM_SIZE && rom_size != CLEAN_ROM_SIZE + HEADER_SIZE) {
			spdlog::warn(fmt::format(colors::WARNING, 
				"Your clean ROM at '{}' is not actually clean, as it has an incorrect size", clean_rom_path.string()));
			return;
		}

		auto checksum_location{ CHECKSUM_LOCATION };
		auto complement_checksum_location{ CHECKSUM_COMPLEMENT_LOCATION };

		if (rom_size == CLEAN_ROM_SIZE + HEADER_SIZE) {
			checksum_location += HEADER_SIZE;
			complement_checksum_location += HEADER_SIZE;
		}

		std::ifstream rom_file(clean_rom_path, std::ios::binary);

		rom_file.seekg(checksum_location);
		char low_checksum;
		char high_checksum;
		rom_file.read(&low_checksum, 1);
		rom_file.read(&high_checksum, 1);
		const auto checksum{ static_cast<unsigned char>(low_checksum) | static_cast<unsigned char>(high_checksum) << 8 };

		rom_file.seekg(complement_checksum_location);
		char low_complement;
		char high_complement;
		rom_file.read(&low_complement, 1);
		rom_file.read(&high_complement, 1);
		const auto complement{ static_cast<unsigned char>(low_complement) | static_cast<unsigned char>(high_complement) << 8 };

		if (checksum != CLEAN_ROM_CHECKSUM || complement != CLEAN_ROM_CHECKSUM_COMPLEMENT) {
			spdlog::warn("Your clean ROM at '{}' is not actually clean, as it has an incorrect checksum", clean_rom_path.string());
		}

		const auto rom_start{ rom_size == CLEAN_ROM_SIZE + HEADER_SIZE ? HEADER_SIZE : 0x0 };

		rom_file.seekg(rom_start);
		uint16_t sum{ 0 };
		for (auto i{ rom_start }; i != rom_size; ++i) {
			if (i == checksum_location || i == checksum_location + 1) {
				rom_file.seekg(1, std::ios::cur);
				continue;
			}
			else if (i == complement_checksum_location || i == complement_checksum_location + 1) {
				sum += 0xFF;
				rom_file.seekg(1, std::ios::cur);
				continue;
			}
			
			char byte;
			rom_file.read(&byte, 1);
			sum += static_cast<unsigned char>(byte);
		}

		if (sum != checksum) {
			spdlog::warn("Your clean ROM at '{}' is not actually clean, as its checksum differs from the sum of its bytes", clean_rom_path.string());
		}
	}

	void Builder::writeIfDifferent(const std::string& str, const fs::path& out_file) {
		std::ifstream in_file{ out_file };

		std::string content((std::istreambuf_iterator<char>(in_file)),
			(std::istreambuf_iterator<char>()));

		in_file.close();

		if (content != str) {
			std::ofstream out{ out_file };
			out << str;
			out.close();
		}
	}

	void Builder::removeBuildReport(const fs::path& project_root) {
		const auto path{ PathUtil::getBuildReportPath(project_root) };
		try {
			if (fs::exists(path)) {
				fs::remove(path);
			}
		}
		catch (const std::exception& e) {
			spdlog::warn("Failed to remove previous build report with the "
				"following exception, Quickbuild may behave erroneously:\n\r{}");
		}
	}
}
