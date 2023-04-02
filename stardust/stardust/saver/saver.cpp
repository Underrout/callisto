#include "saver.h"

namespace stardust {
	using namespace stardust::extractables;

	const std::unordered_map<ExtractableType, Symbol> Saver::extractable_to_symbol = {
		{ ExtractableType::BINARY_MAP16, Symbol::MAP16 },
		{ ExtractableType::TEXT_MAP16, Symbol::MAP16 },
		{ ExtractableType::CREDITS, Symbol::CREDITS },
		{ ExtractableType::GLOBAL_EX_ANIMATION, Symbol::GLOBAL_EX_ANIMATION },
		{ ExtractableType::OVERWORLD, Symbol::OVERWORLD },
		{ ExtractableType::TITLE_SCREEN, Symbol::TITLE_SCREEN },
		{ ExtractableType::LEVELS, Symbol::LEVEL},
		{ ExtractableType::SHARED_PALETTES, Symbol::SHARED_PALETTES }
	};

	std::vector<ExtractableType> Saver::getExtractableTypes(const Configuration& config) {
		std::vector<ExtractableType> extractables{};

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

		if (config.levels.isSet()) {
			extractables.push_back(ExtractableType::LEVELS);
		}

		if (config.shared_palettes.isSet()) {
			extractables.push_back(ExtractableType::SHARED_PALETTES);
		}

		return extractables;
	}

	std::shared_ptr<Extractable> Saver::extractableTypeToExtractable(const Configuration& config, ExtractableType type) {
		const auto extracting_rom{ config.project_rom.getOrThrow() };

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
		case ExtractableType::EX_GRAPHICS:
		default:
			throw StardustException("Unknown extractable type passed");
		}
	}

	std::vector<std::shared_ptr<Extractable>> Saver::getExtractables(const Configuration& config, 
		const std::vector<ExtractableType>& extractable_types) {
		std::vector<std::shared_ptr<Extractable>> extractables{};

		for (const auto& extractable_type : extractable_types) {
			extractables.push_back(extractableTypeToExtractable(config, extractable_type));
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
		Marker::insertMarkerString(rom_path, getExtractableTypes(config));
	}

	void Saver::exportResources(const fs::path& rom_path, const Configuration& config) {
		const auto need_extraction{ Marker::getNeededExtractions(rom_path, 
			getExtractableTypes(config), config.use_text_map16_format.getOrDefault(false)) };
		const auto extractables{ getExtractables(config, need_extraction) };

		for (const auto& extractable : extractables) {
			extractable->extract();
		}

		const auto potential_build_report{ PathUtil::getBuildReportPath(config.project_root.getOrThrow()) };

		if (fs::exists(potential_build_report)) {
			updateBuildReport(potential_build_report, need_extraction);
		}

		Saver::writeMarkerToRom(rom_path, config);
	}
}
