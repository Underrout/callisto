#include "exgraphics.h"

namespace stardust {
	const char* ExGraphics::EXGRAPHICS_FOLDER_NAME = "ExGraphics";

	ExGraphics::ExGraphics(const Configuration& config)
		: LunarMagicInsertable(config),
		project_exgraphics_folder_path(config.project_rom.getOrThrow().parent_path() / EXGRAPHICS_FOLDER_NAME),
		temporary_exgraphics_folder_path(config.temporary_rom.getOrThrow().parent_path() / EXGRAPHICS_FOLDER_NAME)
	{
		registerConfigurationDependency(config.project_rom, Policy::REINSERT);

		if (!fs::exists(project_exgraphics_folder_path)) {
			throw ResourceNotFoundException(fmt::format(
				"ExGraphics folder not found at {}",
				project_exgraphics_folder_path.string()
			));
		}
	}

	void ExGraphics::createTemporaryExGraphicsFolder() const {
		if (temporary_exgraphics_folder_path != project_exgraphics_folder_path) {
			spdlog::debug(fmt::format(
				"Copying project ExGraphics folder {} to temporary ExGraphics folder {}",
				project_exgraphics_folder_path.string(),
				temporary_exgraphics_folder_path.string()
			));
			try {
				fs::copy(project_exgraphics_folder_path, temporary_exgraphics_folder_path, fs::copy_options::overwrite_existing);
			}
			catch (const fs::filesystem_error&) {
				throw InsertionException(fmt::format(
					"Failed to copy project ExGraphics folder {} to temporary ExGraphics folder {}",
					project_exgraphics_folder_path.string(),
					temporary_exgraphics_folder_path.string()
				));
			}
		}
		else {
			spdlog::debug(fmt::format(
				"Project output ROM and temporary ROM are in same folder, no need to copy ExGraphics folder"
			));
		}
	}

	void ExGraphics::deleteTemporaryExGraphicsFolder() const {
		if (temporary_exgraphics_folder_path != project_exgraphics_folder_path) {
			spdlog::debug(fmt::format(
				"Deleting temporary ExGraphics folder {}",
				temporary_exgraphics_folder_path.string()
			));
			try {
				fs::remove_all(temporary_exgraphics_folder_path);
			}
			catch (const fs::filesystem_error&) {
				spdlog::warn(fmt::format(
					"Failed to delete temporary ExGraphics folder {}",
					project_exgraphics_folder_path.string(),
					temporary_exgraphics_folder_path.string()
				));
			}
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

		createTemporaryExGraphicsFolder();

		spdlog::info("Inserting ExGraphics");
		spdlog::debug(fmt::format(
			"Inserting ExGraphics from folder {} into temporary ROM {}",
			project_exgraphics_folder_path.string(),
			temporary_rom_path.string()
		));

		const auto exit_code{ callLunarMagic("-ImportExGFX", temporary_rom_path.string())};

		deleteTemporaryExGraphicsFolder();

		if (exit_code == 0) {
			spdlog::info("Successfully inserted ExGraphics!");
			spdlog::debug(fmt::format(
				"Successfully inserted ExGraphics from folder {} into temporary ROM {}",
				project_exgraphics_folder_path.string(),
				temporary_rom_path.string()
			));
		}
		else {
			throw InsertionException(fmt::format(
				"Failed to insert ExGraphics from folder {} into temporary ROM {}",
				project_exgraphics_folder_path.string(),
				temporary_rom_path.string()
			));
		}
	}
}
