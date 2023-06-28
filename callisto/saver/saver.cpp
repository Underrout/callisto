#include "saver.h"

namespace callisto {
	using namespace callisto::extractables;

	const std::unordered_map<ExtractableType, Symbol> Saver::extractable_to_symbol = {
		{ ExtractableType::BINARY_MAP16, Symbol::MAP16 },
		{ ExtractableType::TEXT_MAP16, Symbol::MAP16 },
		{ ExtractableType::CREDITS, Symbol::CREDITS },
		{ ExtractableType::GLOBAL_EX_ANIMATION, Symbol::GLOBAL_EX_ANIMATION },
		{ ExtractableType::OVERWORLD, Symbol::OVERWORLD },
		{ ExtractableType::TITLE_SCREEN, Symbol::TITLE_SCREEN },
		{ ExtractableType::LEVELS, Symbol::LEVELS},
		{ ExtractableType::SHARED_PALETTES, Symbol::SHARED_PALETTES }
	};

	std::vector<ExtractableType> Saver::getExtractableTypes(const Configuration& config) {
		std::vector<ExtractableType> extractables{};

		if (config.levels.isSet()) {
			extractables.push_back(ExtractableType::LEVELS);
		}

		if (config.map16.isSet()) {
			if (config.use_text_map16_format.getOrDefault(false)) {
				extractables.push_back(ExtractableType::TEXT_MAP16);
			}
			else {
				extractables.push_back(ExtractableType::BINARY_MAP16);
			}
		}

		if (config.credits.isSet()) {
			extractables.push_back(ExtractableType::CREDITS);
		}

		if (config.global_exanimation.isSet()) {
			extractables.push_back(ExtractableType::GLOBAL_EX_ANIMATION);
		}

		if (config.overworld.isSet()) {
			extractables.push_back(ExtractableType::OVERWORLD);
		}

		if (config.titlescreen.isSet()) {
			extractables.push_back(ExtractableType::TITLE_SCREEN);
		}

		if (config.shared_palettes.isSet()) {
			extractables.push_back(ExtractableType::SHARED_PALETTES);
		}

		if (!config.build_order.empty()) {
			bool graphics_seen{ false };
			bool exgraphics_seen{ false };
			for (const auto& descriptor : config.build_order) {
				if (graphics_seen && exgraphics_seen) {
					break;
				}
				if (descriptor.symbol == Symbol::GRAPHICS && !graphics_seen) {
					graphics_seen = true;
					const auto graphics_folder{ GraphicsUtil::getExportFolderPath(config, false) };
					if (!fs::exists(graphics_folder)) {
						// only extract graphics if there is no graphics folder, otherwise, assume that the graphics in the folder
						// may be more up to date than the ones in the ROM and shouldn't be overwritten
						extractables.push_back(ExtractableType::GRAPHICS);
					}
					else {
						const auto link{ GraphicsUtil::getLunarMagicFolderPath(config.project_rom.getOrThrow(), false) };
						if (!fs::exists(link)) {
							GraphicsUtil::createSymlink(link, graphics_folder);
						}
					}
				}
				else if (descriptor.symbol == Symbol::EX_GRAPHICS && !exgraphics_seen) {
					exgraphics_seen = true;
					const auto exgraphics_folder{ GraphicsUtil::getExportFolderPath(config, true) };
					if (!fs::exists(exgraphics_folder)) {
						extractables.push_back(ExtractableType::EX_GRAPHICS);
					}
					else {
						const auto link{ GraphicsUtil::getLunarMagicFolderPath(config.project_rom.getOrThrow(), true) };
						if (!fs::exists(link)) {
							GraphicsUtil::createSymlink(link, exgraphics_folder);
						}
					}
				}
			}
		}

		return extractables;
	}

	std::shared_ptr<Extractable> Saver::extractableTypeToExtractable(const Configuration& config, ExtractableType type, const fs::path& extracting_rom) {
		switch (type) {
		case ExtractableType::BINARY_MAP16:
			return std::make_shared<BinaryMap16>(config, extracting_rom);
		case ExtractableType::TEXT_MAP16:
			return std::make_shared<TextMap16>(config, extracting_rom);
		case ExtractableType::CREDITS:
			return std::make_shared<Credits>(config, extracting_rom);
		case ExtractableType::GLOBAL_EX_ANIMATION:
			return std::make_shared<GlobalExAnimation>(config, extracting_rom);
		case ExtractableType::OVERWORLD:
			return std::make_shared<Overworld>(config, extracting_rom);
		case ExtractableType::TITLE_SCREEN:
			return std::make_shared<TitleScreen>(config, extracting_rom);
		case ExtractableType::LEVELS:
			return std::make_shared<Levels>(config, extracting_rom);
		case ExtractableType::SHARED_PALETTES:
			return std::make_shared<SharedPalettes>(config, extracting_rom);
		case ExtractableType::GRAPHICS:
			return std::make_shared<Graphics>(config, extracting_rom);
		case ExtractableType::EX_GRAPHICS:
			return std::make_shared<ExGraphics>(config, extracting_rom);
		default:
			throw CallistoException("Unknown extractable type passed");
		}
	}

	std::vector<std::shared_ptr<Extractable>> Saver::getExtractables(const Configuration& config, 
		const std::vector<ExtractableType>& extractable_types, const fs::path& extracting_rom) {
		std::vector<std::shared_ptr<Extractable>> extractables{};

		for (const auto& extractable_type : extractable_types) {
			extractables.push_back(extractableTypeToExtractable(config, extractable_type, extracting_rom));
		}

		return extractables;
	}

	void Saver::updateBuildReport(const fs::path& build_report, const std::vector<ExtractableType>& extracted_types) {
		std::ifstream file{ build_report };
		json j{ json::parse(file) };

		std::unordered_set<Symbol> extracted_symbols{};
		for (const auto& extracted_type : extracted_types) {
			extracted_symbols.insert(extractable_to_symbol.at(extracted_type));
		}

		for (auto& entry : j["dependencies"]) {
			Descriptor descriptor{ entry["descriptor"] };
			if (extracted_symbols.find(descriptor.symbol) != extracted_symbols.end()) {
				for (auto& json_resource_dependency : entry["resource_dependencies"]) {
					ResourceDependency dependency{ json_resource_dependency };
					ResourceDependency new_dependency{ dependency.dependent_path, dependency.policy };
					if (new_dependency.last_write_time.has_value()) {
						json_resource_dependency["timestamp"] = new_dependency.last_write_time.value();
					}
					else {
						json_resource_dependency["timestamp"] = nullptr;
					}
				}
			}
		}

		std::ofstream out_build_report{ build_report };
		out_build_report << std::setw(4) << j;
		out_build_report.close();
	}

	void Saver::writeMarkerToRom(const fs::path& rom_path, const Configuration& config) {
		const auto now{ std::chrono::system_clock::now() };
		auto timestamp{ std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() };
		spdlog::debug("Marker timestamp is {:010X}", timestamp);

		Marker::insertMarkerString(rom_path, getExtractableTypes(config), timestamp);

		fs::last_write_time(rom_path, std::chrono::clock_cast<std::chrono::file_clock>(now));

		// little uggo to put this here, but I'm essentially storing the timestamp also to a cache file
		// so that if the ROM is switched with a different ROM that was built at a different time, we can still tell
		const auto project_root{ config.project_root.getOrThrow() };
		const auto last_rom_sync_path{ PathUtil::getLastRomSyncPath(project_root) };

		json j;
		j["last_sync_time"] = timestamp;

		std::ofstream sync_file{ last_rom_sync_path };
		sync_file << std::setw(4) << j << std::endl;
		sync_file.close();
	}

	void Saver::exportResources(const fs::path& rom_path, const Configuration& config, bool force, bool mark) {
		std::vector<ExtractableType> need_extraction;
		if (!force) {
			need_extraction = Marker::getNeededExtractions(rom_path, config.project_root.getOrThrow(),
				getExtractableTypes(config), config.use_text_map16_format.getOrDefault(false));
		}
		else {
			need_extraction = getExtractableTypes(config);
		}

		if (!need_extraction.empty()) {
			if (!force) {
				spdlog::info("Some resources need to be exported, exporting them now");
			}
			else {
				spdlog::info("Exporting all resources");
			}
			const auto export_start{ std::chrono::high_resolution_clock::now() };

			const auto extractables{ getExtractables(config, need_extraction, rom_path) };

			std::exception_ptr thread_exception{};
			std::for_each(std::execution::par, extractables.begin(), extractables.end(), [&](auto&& extractable) { 
				try {
					extractable->extract(); 
				}
				catch (...) {
					thread_exception = std::current_exception();
				}
			});

			if (thread_exception != nullptr) {
				std::rethrow_exception(thread_exception);
			}

			if (mark) {
				const auto potential_build_report{ PathUtil::getBuildReportPath(config.project_root.getOrThrow()) };

				if (fs::exists(potential_build_report)) {
					spdlog::info("Found a build report, updating it now");
					try {
						updateBuildReport(potential_build_report, need_extraction);
						spdlog::info("Successfully updated build report!");
					}
					catch (const std::exception& e) {
						spdlog::warn("Failed to update build report with exception:\n{}", e.what());
					}
				}
				spdlog::info("Writing export marker to ROM");
				try {
					Saver::writeMarkerToRom(rom_path, config);
					spdlog::info("Successfully wrote export marker to ROM!");
				}
				catch (const std::exception& e) {
					spdlog::warn("Failed to write export marker to ROM with exception:\n{}", e.what());
				}
			}

			const auto export_end{ std::chrono::high_resolution_clock::now() };

			spdlog::info("All resources exported successfully in {}!", TimeUtil::getDurationString(export_end - export_start));
		}
		else {
			spdlog::info("All resources already exported!");
		}
	}
}
