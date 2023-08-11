#include "exgraphics.h"

namespace callisto {
	ExGraphics::ExGraphics(const Configuration& config)
		: LunarMagicInsertable(config),
		project_exgraphics_folder_path(GraphicsUtil::getExportFolderPath(config, true)),
		config(config)
	{
		if (config.ex_graphics.isSet()) {
			registerConfigurationDependency(config.ex_graphics, Policy::REINSERT);
		}
		else {
			registerConfigurationDependency(config.output_rom, Policy::REINSERT);
		}

		if (!fs::exists(project_exgraphics_folder_path)) {
			throw ResourceNotFoundException(fmt::format(
				colors::build::EXCEPTION,
				"ExGraphics folder not found at {}",
				project_exgraphics_folder_path.string()
			));
		}
	}

	std::unordered_set<ResourceDependency> ExGraphics::determineDependencies() {
		auto dependencies{ LunarMagicInsertable::determineDependencies() };
		const auto folder_dependencies{
			getResourceDependenciesFor(project_exgraphics_folder_path, Policy::REINSERT)
		};
		dependencies.insert(folder_dependencies.begin(), folder_dependencies.end());
		return dependencies;
	}

	void ExGraphics::insert() {
		if (!fs::exists(project_exgraphics_folder_path)) {
			throw InsertionException(fmt::format("No ExGraphics folder found at {}", project_exgraphics_folder_path.string()));
		}

		spdlog::info(fmt::format(colors::build::REMARK, "Inserting ExGraphics"));
		spdlog::debug(fmt::format(
			"Inserting ExGraphics from folder {} into temporary ROM {}",
			project_exgraphics_folder_path.string(),
			temporary_rom_path.string()
		));

		try {
			GraphicsUtil::importProjectExGraphicsInto(config, temporary_rom_path);
		}
		catch (const std::exception& e) {
			throw InsertionException(fmt::format(
				colors::build::EXCEPTION,
				"Failed to insert ExGraphics from folder {} into temporary ROM {} with following exception:\n\r{}",
				project_exgraphics_folder_path.string(),
				temporary_rom_path.string(),
				e.what()
			));
		}

		spdlog::info(fmt::format(colors::build::PARTIAL_SUCCESS, "Successfully inserted ExGraphics!"));
		spdlog::debug(fmt::format(
			"Successfully inserted ExGraphics from folder {} into temporary ROM {}",
			project_exgraphics_folder_path.string(),
			temporary_rom_path.string()
		));
	}
}
