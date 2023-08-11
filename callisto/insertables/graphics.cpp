#include "graphics.h"

namespace callisto {
	Graphics::Graphics(const Configuration& config)
		: LunarMagicInsertable(config), 
		project_graphics_folder_path(GraphicsUtil::getExportFolderPath(config, false)),
		config(config)
	{
		if (config.graphics.isSet()) {
			registerConfigurationDependency(config.graphics, Policy::REINSERT);
		}
		else {
			registerConfigurationDependency(config.output_rom, Policy::REINSERT);
		}

		if (!fs::exists(project_graphics_folder_path)) {
			throw ResourceNotFoundException(fmt::format(
				colors::build::EXCEPTION,
				"Graphics folder not found at {}",
				project_graphics_folder_path.string()
			));
		}
	}

	std::unordered_set<ResourceDependency> Graphics::determineDependencies() {
		auto dependencies{ LunarMagicInsertable::determineDependencies() };
		const auto folder_dependencies{
			getResourceDependenciesFor(project_graphics_folder_path, Policy::REINSERT)
		};
		dependencies.insert(folder_dependencies.begin(), folder_dependencies.end());
		return dependencies;
	}

	void Graphics::insert() {
		if (!fs::exists(project_graphics_folder_path)) {
			throw InsertionException(fmt::format(colors::build::EXCEPTION, "No Graphics folder found at {}", project_graphics_folder_path.string()));
		}

		spdlog::info(fmt::format(colors::build::REMARK, "Inserting Graphics"));
		spdlog::debug(fmt::format(
			"Inserting Graphics from folder {} into temporary ROM {}",
			project_graphics_folder_path.string(),
			temporary_rom_path.string()
		));

		try {
			GraphicsUtil::importProjectGraphicsInto(config, temporary_rom_path);
		}
		catch (const std::exception& e) {
			throw InsertionException(fmt::format(
				colors::build::EXCEPTION,
				"Failed to insert Graphics from folder {} into temporary ROM {} with following exception:\n\r{}",
				project_graphics_folder_path.string(),
				temporary_rom_path.string(),
				e.what()
			));
		}

		spdlog::info(fmt::format(colors::build::PARTIAL_SUCCESS, "Successfully inserted Graphics!"));
		spdlog::debug(fmt::format(
			"Successfully inserted Graphics from folder {} into temporary ROM {}",
			project_graphics_folder_path.string(),
			temporary_rom_path.string()
		));
	}
}
