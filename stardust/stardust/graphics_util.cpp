#include "graphics_util.h"

namespace stardust {
	void GraphicsUtil::exportResources(const Configuration& config, const fs::path& rom_path, bool exgfx) {
		const auto final_output_path{ getExportFolderPath(config, exgfx) };
		const auto intermediate_output_path{ 
			getLunarMagicFolderPath(rom_path, exgfx) };

		fs::create_directories(final_output_path);

		if (final_output_path != intermediate_output_path) {
			createSymlink(intermediate_output_path, final_output_path);
		}

		const auto command{ getExportCommand(exgfx) };
		const auto exit_code{ callLunarMagic(config, getExportCommand(exgfx),
			rom_path.string()) };

		if (exit_code != 0) {
			throw ExtractionException(fmt::format("Failed to export {} from ROM {} to {}",
				exgfx ? "ExGraphics" : "Graphics",
				rom_path.string(),
				final_output_path.string()
			));
		}
	}

	void GraphicsUtil::importResources(const Configuration& config, const fs::path& rom_path, bool exgfx) {
		const auto source_path{ getExportFolderPath(config, exgfx) };
		const auto import_path{ getLunarMagicFolderPath(rom_path, exgfx) };

		if (source_path != import_path) {
			if (fs::exists(import_path)) {
				fs::remove_all(import_path);
			}

			createSymlink(import_path, source_path);
		}

		const auto command{ getImportCommand(exgfx) };
		const auto exit_code{ callLunarMagic(config, command, rom_path.string()) };

		deleteSymlink(import_path);

		if (exit_code != 0) {
			throw InsertionException(fmt::format("Failed to import {} from {} into ROM {}",
				exgfx ? "ExGraphics" : "Graphics",
				source_path.string(),
				rom_path.string()
			));
		}
	}

	void GraphicsUtil::importProjectGraphicsInto(const Configuration& config, const fs::path& rom_path) {
		importResources(config, rom_path, false);
	}

	void GraphicsUtil::importProjectExGraphicsInto(const Configuration& config, const fs::path& rom_path) {
		importResources(config, rom_path, true);
	}

	void GraphicsUtil::exportProjectGraphicsFrom(const Configuration& config, const fs::path& rom_path) {
		exportResources(config, rom_path, false);
	}

	void GraphicsUtil::exportProjectExGraphicsFrom(const Configuration& config, const fs::path& rom_path) {
		exportResources(config, rom_path, true);
	}
}
