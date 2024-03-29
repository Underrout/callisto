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
		checkLunarMagicExists();

		if (!fs::exists(project_exgraphics_folder_path)) {
			throw ResourceNotFoundException(fmt::format(
				colors::EXCEPTION,
				"ExGraphics folder not found at {}",
				project_exgraphics_folder_path.string()
			));
		}

		if (!fs::exists(project_exgraphics_folder_path)) {
			throw InsertionException(fmt::format("No ExGraphics folder found at {}", project_exgraphics_folder_path.string()));
		}

		spdlog::info(fmt::format(colors::RESOURCE, "Inserting ExGraphics"));
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
				colors::EXCEPTION,
				"Failed to insert ExGraphics from folder {} into temporary ROM {} with following exception:\n\r{}",
				project_exgraphics_folder_path.string(),
				temporary_rom_path.string(),
				e.what()
			));
		}

		spdlog::info(fmt::format(colors::PARTIAL_SUCCESS, "Successfully inserted ExGraphics!"));
		spdlog::debug(fmt::format(
			"Successfully inserted ExGraphics from folder {} into temporary ROM {}",
			project_exgraphics_folder_path.string(),
			temporary_rom_path.string()
		));
	}
}
