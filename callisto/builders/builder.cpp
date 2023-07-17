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

		if (symbol == Symbol::GRAPHICS) {
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
		else if (symbol == Symbol::GLOBULE) {
			const auto callisto_directory{ PathUtil::getCallistoDirectoryPath(config.project_root.getOrThrow()) };
			std::vector<fs::path> include_paths{ callisto_directory };

			return std::make_shared<Globule>(
				config,
				name.value(),
				PathUtil::getGlobuleImprintDirectoryPath(config.project_root.getOrThrow()),
				PathUtil::getGlobuleCallFilePath(config.project_root.getOrThrow()),
				config.globules.getOrDefault({}),
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
			throw ConfigException("Unknown build order symbol");
		}
	}

	json Builder::createBuildReport(const Configuration& config, const json& dependency_report) {
		spdlog::info("Creating build report");
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
						"Failed to determine source level number of level file '{}' with exception:\n\r{}",
						entry.path().string(), e.what()
					));
				}
			}
		}

		return report;
	}

	void Builder::writeBuildReport(const fs::path& project_root, const json& j) {
		spdlog::info("Writing build report");
		std::ofstream build_report{ PathUtil::getBuildReportPath(project_root) };
		build_report << std::setw(4) << j << std::endl;
		build_report.close();
	}

	void Builder::cacheGlobules(const fs::path& project_root) {
		spdlog::info("Caching globules");
		const auto source{ PathUtil::getGlobuleImprintDirectoryPath(project_root) };
		const auto target{ PathUtil::getInsertedGlobulesDirectoryPath(project_root) };
		fs::create_directories(target);
		if (fs::exists(source)) {
			fs::remove_all(target);
			fs::copy(source, target, fs::copy_options::overwrite_existing);
			fs::remove_all(source);
		}
	}

	void Builder::moveTempToOutput(const Configuration& config) {
		spdlog::info("Moving temporary files to final output");
		for (const auto& entry : fs::directory_iterator(config.temporary_rom.getOrThrow().parent_path())) {
			if (fs::is_regular_file(entry)) {
				const auto file_name{ entry.path().stem().string() };
				const auto temporary_rom_name{ config.temporary_rom.getOrThrow().stem().string() };
				if (file_name.substr(0, temporary_rom_name.size()) == temporary_rom_name) {
					const auto source{ entry.path() };
					const auto target{ config.project_rom.getOrThrow().parent_path() / 
						(config.project_rom.getOrThrow().stem().string() + entry.path().extension().string())};

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
								spdlog::error("Failed to copy '{}' to '{}', try again? Y/N", source.string(), target.string());
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
						spdlog::warn("Failed to remove temporary file '{}'", source.string());
					}
				}
			}
		}
	}
	
	void Builder::init(const Configuration& config) {
		spdlog::info("Initializing callisto directory");
		ensureCacheStructure(config);
		generateAssemblyLevelInformation(config);
		generateGlobuleCallFile(config);
		if (config.temporary_rom.isSet()) {
			fs::create_directories(config.temporary_rom.getOrThrow().parent_path());
		}
		if (config.project_rom.isSet()) {
			fs::create_directories(config.project_rom.getOrThrow().parent_path());
		}

		if (config.levels.isSet()) {
			spdlog::info("Ensuring normalized level filenames");
			Levels::normalizeMwls(config.levels.getOrThrow(), config.allow_user_input);
		}
	}

	void Builder::ensureCacheStructure(const Configuration& config) {
		const auto project_root{ config.project_root.getOrThrow() };
		fs::remove_all(PathUtil::getGlobuleImprintDirectoryPath(project_root));
		fs::create_directories(PathUtil::getGlobuleImprintDirectoryPath(project_root));
		fs::create_directories(PathUtil::getInsertedGlobulesDirectoryPath(project_root));
	}

	void Builder::generateAssemblyLevelInformation(const Configuration& config) {
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
			"!{}_{}_{} = {}\n",
			DEFINE_PREFIX, PROFILE_DEFINE_NAME, config.config_name.getOrThrow(),
			DEFINE_PREFIX, ASSEMBLING_DEFINE_NAME,
			DEFINE_PREFIX, VERSION_DEFINE_NAME, CALLISTO_VERSION_MAJOR, CALLISTO_VERSION_MINOR, CALLISTO_VERSION_PATCH,
			DEFINE_PREFIX, VERSION_DEFINE_NAME, MAJOR_VERSION_DEFINE_NAME, CALLISTO_VERSION_MAJOR,
			DEFINE_PREFIX, VERSION_DEFINE_NAME, MINOR_VERSION_DEFINE_NAME, CALLISTO_VERSION_MINOR,
			DEFINE_PREFIX, VERSION_DEFINE_NAME, PATCH_VERSION_DEFINE_NAME, CALLISTO_VERSION_PATCH
		) };

		writeIfDifferent(info_string, PathUtil::getAssemblyInfoFilePath(config.project_root.getOrThrow()));
	}

	void Builder::generateGlobuleCallFile(const Configuration& config) {
		std::string call{ "includeonce\n\nmacro call(globule_label)\n\tPHB\n\t"
			"LDA.b #<globule_label>>>16\n\tPHA\n\tPLB\n\tJSL <globule_label>\n\tPLB\nendmacro\n" };

		writeIfDifferent(call, PathUtil::getGlobuleCallFilePath(config.project_root.getOrThrow()));
	}

	void Builder::checkCleanRom(const fs::path& clean_rom_path) {
		if (!fs::exists(clean_rom_path)) {
			throw InsertionException(fmt::format(
				"No clean ROM found at '{}'", clean_rom_path.string()
			));
		}

		if (clean_rom_path.extension() != ".smc") {
			spdlog::warn("Your clean ROM at '{}' does not have a .smc extension", clean_rom_path.string());
		}

		const auto rom_size{ fs::file_size(clean_rom_path) };		
		if (rom_size != CLEAN_ROM_SIZE && rom_size != CLEAN_ROM_SIZE + HEADER_SIZE) {
			spdlog::warn("Your clean ROM at '{}' is not actually clean, as it has an incorrect size", clean_rom_path.string());
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
