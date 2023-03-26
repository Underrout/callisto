#include "builder.h"

namespace stardust {
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
		else if (symbol == Symbol::PIXI) {
			return std::make_shared<Pixi>(config);
		}
		else if (symbol == Symbol::PATCH) {
			std::vector<fs::path> include_paths{ PathUtil::getStardustCache(config.project_root.getOrThrow()) };

			return std::make_shared<Patch>(
				config,
				name.value(),
				include_paths
			);
		} 
		else if (symbol == Symbol::GLOBULE) {
			const auto stardust_cache{ PathUtil::getStardustCache(config.project_root.getOrThrow()) };
			std::vector<fs::path> include_paths{ stardust_cache };

			return std::make_shared<Globule>(
				config,
				name.value(),
				stardust_cache / "globules",
				stardust_cache / "call.asm",
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
		else if (symbol == Symbol::LEVEL) {
			return std::make_shared<Level>(
				config,
				name.value()
			);
		}
		else {
			throw ConfigException("Unknown build order symbol");
		}
	}

	json Builder::createBuildReport(const Configuration& config, const json& dependency_report) {
		json report{};
		report["dependencies"] = dependency_report;
		report["configuration"] = config.config_name.getOrThrow();
		report["file_format_version"] = BUILD_REPORT_VERSION;
		report["build_order"] = std::vector<std::string>();
		report["rom_size"] = config.rom_size.isSet() ? config.rom_size.getOrThrow() : nullptr;

		for (const auto& descriptor : config.build_order) {
			report["build_order"].push_back(descriptor.toJson());
		}

		return report;
	}

	void Builder::writeBuildReport(const fs::path& project_root, const json& j) {
		const auto path{ PathUtil::getStardustCache(project_root) / ".cache" };
		fs::create_directories(path);
		std::ofstream build_report{ path / "build_report.json" };
		build_report << std::setw(4) << j << std::endl;
		build_report.close();
	}

	void Builder::cacheGlobules(const fs::path& project_root) {
		const auto source{ PathUtil::getStardustCache(project_root) / "globules" };
		const auto target{ PathUtil::getStardustCache(project_root) / ".cache" / "inserted_globules" };
		fs::create_directories(target);
		if (fs::exists(source)) {
			fs::remove_all(target);
			fs::copy(source, target, fs::copy_options::overwrite_existing);
			fs::remove_all(source);
		}
	}

	void Builder::moveTempToOutput(const Configuration& config) {
		for (const auto& entry : fs::directory_iterator(config.temporary_rom.getOrThrow().parent_path())) {
			if (fs::is_regular_file(entry)) {
				const auto file_name{ entry.path().stem().string() };
				const auto temporary_rom_name{ config.temporary_rom.getOrThrow().stem().string() };
				if (file_name.substr(0, temporary_rom_name.size()) == temporary_rom_name) {
					const auto source{ entry.path() };
					const auto target{ config.project_rom.getOrThrow().parent_path() / 
						(config.project_rom.getOrThrow().stem().string() + entry.path().extension().string())};
					fs::copy_file(source, target, fs::copy_options::overwrite_existing);
					fs::remove(source);
				}
			}
		}
	}
}
