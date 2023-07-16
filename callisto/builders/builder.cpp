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
				"following exception, Quickbuild may behave erroneously:\n{}");
		}
	}
}
