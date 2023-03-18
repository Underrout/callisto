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
			return std::make_shared<Graphics>(
				config.lunar_magic_path.getOrThrow(),
				config.temporary_rom.getOrThrow(),
				config.project_rom.getOrThrow()
			);
		}
		else if (symbol == Symbol::EX_GRAPHICS) {
			return std::make_shared<ExGraphics>(
				config.lunar_magic_path.getOrThrow(),
				config.temporary_rom.getOrThrow(),
				config.project_rom.getOrThrow()
			);
		}
		else if (symbol == Symbol::MAP16) {
			if (config.use_text_map16_format.getOrDefault(false)) {
				return std::make_shared<TextMap16>(
					config.lunar_magic_path.getOrThrow(),
					config.temporary_rom.getOrThrow(),
					config.map16.getOrThrow(),
					"." // TODO make it so the map16 conversion tool is integrated
				);
			}
			else {
				return std::make_shared<BinaryMap16>(
					config.lunar_magic_path.getOrThrow(),
					config.temporary_rom.getOrThrow(),
					config.map16.getOrThrow()
				);
			}
		}
		else if (symbol == Symbol::TITLE_SCREEN_MOVEMENT) {
			return { std::make_shared<TitleMoves>(
				config.lunar_magic_path.getOrThrow(),
				config.temporary_rom.getOrThrow(),
				config.title_moves.getOrThrow()
			) };
		}
		else if (symbol == Symbol::SHARED_PALETTES) {
			return std::make_shared<SharedPalettes>(
				config.lunar_magic_path.getOrThrow(),
				config.temporary_rom.getOrThrow(),
				config.shared_palettes.getOrThrow()
			);
		}
		else if (symbol == Symbol::OVERWORLD) {
			return std::make_shared<Overworld>(
				config.flips_path.getOrThrow(),
				config.clean_rom.getOrThrow(),
				config.lunar_magic_path.getOrThrow(),
				config.temporary_rom.getOrThrow(),
				config.overworld.getOrThrow()
			);
		}
		else if (symbol == Symbol::TITLE_SCREEN) {
			return std::make_shared<TitleScreen>(
				config.flips_path.getOrThrow(),
				config.clean_rom.getOrThrow(),
				config.lunar_magic_path.getOrThrow(),
				config.temporary_rom.getOrThrow(),
				config.titlescreen.getOrThrow()
			);
		}
		else if (symbol == Symbol::CREDITS) {
			return std::make_shared<Credits>(
				config.flips_path.getOrThrow(),
				config.clean_rom.getOrThrow(),
				config.lunar_magic_path.getOrThrow(),
				config.temporary_rom.getOrThrow(),
				config.credits.getOrThrow()
			);
		}
		else if (symbol == Symbol::GLOBAL_EX_ANIMATION) {
			return std::make_shared<GlobalExAnimation>(
				config.flips_path.getOrThrow(),
				config.clean_rom.getOrThrow(),
				config.lunar_magic_path.getOrThrow(),
				config.temporary_rom.getOrThrow(),
				config.global_exanimation.getOrThrow()
			);
		}
		else if (symbol == Symbol::PIXI) {
			return std::make_shared<Pixi>(
				config.pixi_working_dir.getOrThrow(),
				config.temporary_rom.getOrThrow(),
				config.pixi_options.getOrDefault(""),
				config.pixi_static_dependencies.getOrDefault({}),
				config.pixi_dependency_report_file.getOrDefault({})
			);
		}
		else if (symbol == Symbol::PATCH) {
			std::vector<fs::path> include_paths{ PathUtil::getStardustCache(config.project_root.getOrThrow()) };

			return std::make_shared<Patch>(
				config.project_root.getOrThrow(),
				config.temporary_rom.getOrThrow(),
				name.value(),
				include_paths
			);
		} 
		else if (symbol == Symbol::GLOBULE) {
			const auto stardust_cache{ PathUtil::getStardustCache(config.project_root.getOrThrow()) };
			std::vector<fs::path> include_paths{ stardust_cache };

			return std::make_shared<Globule>(
				config.project_root.getOrThrow(),
				config.temporary_rom.getOrThrow(),
				name.value(),
				stardust_cache / "globules",
				stardust_cache / "call.asm",
				config.globules.getOrDefault({}),
				config.globule_header.getOrDefault({}),
				include_paths
			);
		}
		else if (symbol == Symbol::EXTERNAL_TOOL) {
			const auto& tool_config{ config.generic_tool_configurations.at(name.value()) };
			return std::make_shared<ExternalTool>(
				name.value(),
				tool_config.executable.getOrThrow(),
				tool_config.options.getOrDefault(""),
				tool_config.working_directory.getOrThrow(),
				config.temporary_rom.getOrThrow(),
				false,
				tool_config.static_dependencies.getOrDefault({}),
				tool_config.dependency_report_file.getOrDefault({})
			);
		}
		else if (symbol == Symbol::LEVEL) {
			return std::make_shared<Level>(
				config.lunar_magic_path.getOrThrow(),
				config.temporary_rom.getOrThrow(), 
				name.value()
			);
		}
		else {
			throw ConfigException("Unknown build order symbol");
		}
	}
}
