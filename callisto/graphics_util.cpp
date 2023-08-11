#include "graphics_util.h"

namespace callisto {
	void GraphicsUtil::exportResources(const Configuration& config, bool exgfx) {
		const auto temporary_export_rom{ 
			PathUtil::getTemporaryRomPath(config.temporary_folder.getOrThrow(), config.output_rom.getOrThrow()) };

		const auto command{ getExportCommand(exgfx) };
		const auto exit_code{ callLunarMagic(config, getExportCommand(exgfx),
			temporary_export_rom.string()) };

		const auto exgfx_or_gfx{ exgfx ? "ExGraphics" : "Graphics" };

		if (exit_code != 0) {
			throw ExtractionException(fmt::format(
				colors::build::EXCEPTION,
				"Failed to export {} from temporary ROM {}",
				exgfx_or_gfx,
				temporary_export_rom.string()
			));
		}

		const auto temporary_exported_folder{ temporary_export_rom.parent_path() / fs::path(exgfx_or_gfx) };
		const auto final_output_path{ getExportFolderPath(config, exgfx) };

		if (!fs::exists(final_output_path)) {
			fs::rename(temporary_exported_folder, final_output_path);
		}
		else {
			fixPotentialExportDiscrepancy(temporary_exported_folder, final_output_path, exgfx, config.allow_user_input);
		}

		const auto original_folder_proxy{ config.output_rom.getOrThrow().parent_path() / fs::path(exgfx_or_gfx) };
		createSymlink(original_folder_proxy, final_output_path);
	}

	void GraphicsUtil::importResources(const Configuration& config, const fs::path& rom_path, bool exgfx) {
		const auto source_path{ getExportFolderPath(config, exgfx) };
		verifyFilenames(source_path, exgfx);

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
			throw InsertionException(fmt::format(
				colors::build::EXCEPTION,
				"Failed to import {} from {} into ROM {}",
				exgfx ? "ExGraphics" : "Graphics",
				source_path.string(),
				rom_path.string()
			));
		}
	}

	void GraphicsUtil::fixPotentialExportDiscrepancy(const fs::path& new_folder, const fs::path& old_folder, bool exgfx, bool allow_user_input) {
		verifyFilenames(old_folder, exgfx);
		if (oldAndNewDiffer(new_folder, old_folder, exgfx)) {
			const auto exgfx_or_gfx{ exgfx ? "ExGFX" : "GFX" };
			if (allow_user_input) {
				PromptUtil::yesNoPrompt(fmt::format(
					colors::build::WARNING,
					"{} stored at '{}' differ from {} stored in ROM, export {} from the ROM anyway?"
					"\n\rWARNING: This will overwrite the files in your {} folder, discarding any "
					"unimported changes made there, only do this if you're sure!!",
					exgfx_or_gfx, old_folder.string(), exgfx_or_gfx, exgfx_or_gfx, exgfx_or_gfx
				), [&] {
					spdlog::info(fmt::format(colors::build::NOTIFICATION, "Overwriting {} at '{}' with {} from ROM",
					exgfx_or_gfx, old_folder.string(), exgfx_or_gfx));
					fs::remove_all(old_folder);
					fs::rename(new_folder, old_folder);
					spdlog::info(fmt::format(colors::build::PARTIAL_SUCCESS, 
						"Successfully overwrote {} at '{}' with {} from ROM", exgfx_or_gfx, old_folder.string(), exgfx_or_gfx));
				});
			}
			else {
				throw ExtractionException(fmt::format(
					colors::build::EXCEPTION,
					"{} stored at '{}' differ from {} stored in ROM, refusing to export",
					exgfx_or_gfx, old_folder.string(), exgfx_or_gfx
				));
			}
		}
	}

	bool GraphicsUtil::oldAndNewDiffer(const fs::path& new_folder, const fs::path& old_folder, bool exgfx) {
		std::vector<fs::path> new_files{};
		for (const auto& entry : fs::directory_iterator(new_folder)) {
			new_files.push_back(entry.path().filename());
		}
		std::sort(new_files.begin(), new_files.end());

		std::vector<fs::path> old_files{};
		for (const auto& entry : fs::directory_iterator(old_folder)) {
			old_files.push_back(entry.path().filename());
		}
		std::sort(old_files.begin(), old_files.end());

		if (new_files.size() != old_files.size()) {
			return true;
		}

		std::vector<fs::path> shared_files{};
		std::set_intersection(new_files.begin(), new_files.end(), 
			old_files.begin(), old_files.end(), std::back_inserter(shared_files));

		std::exception_ptr thread_exception{};
		std::atomic_bool differ{ false };
		std::for_each(std::execution::par, shared_files.begin(), shared_files.end(), [&](const auto& file_path) {
			try {
				if (!differ) {
					const auto old_file{ old_folder / file_path };
					const auto new_file{ new_folder / file_path };
					if (!filesEqual(old_file, new_file)) {
						differ = true;
					}
				}
			}
			catch (...) {
				thread_exception = std::current_exception();
			}
		});

		if (thread_exception != nullptr) {
			try {
				std::rethrow_exception(thread_exception);
			}
			catch (const std::exception& e) {
				throw CallistoException(fmt::format(
					colors::build::EXCEPTION,
					"Failed to compare graphics exported from ROM with project graphics stored in '{}' with exception:\n\r{}", 
					old_folder.string(), e.what()
				));
			}
		}

		return differ;
	}

	bool GraphicsUtil::filesEqual(const fs::path& file1, const fs::path& file2) {
		if (fs::file_size(file1) != fs::file_size(file2)) {
			return false;
		}

		std::ifstream stream1(file1, std::ios::binary);
		std::ifstream stream2(file2, std::ios::binary);

		return std::equal(std::istreambuf_iterator<char>(stream1.rdbuf()), 
			std::istreambuf_iterator<char>(), std::istreambuf_iterator<char>(stream2.rdbuf()));
	}

	void GraphicsUtil::verifyFilenames(const fs::path& graphics_folder, bool exgfx) {
		for (const auto& entry : fs::directory_iterator(graphics_folder)) {
			verifyFilename(entry.path(), exgfx);
		}
	}

	void GraphicsUtil::verifyFilename(const fs::path& file_path, bool exgfx) {
		if (file_path.extension() != ".bin") {
			return;
		}
		
		const std::string start{ exgfx ? "ExGFX" : "GFX" };
		const auto filename{ file_path.stem().string() };

		const VerificationException verification_exception{ fmt::format(
			colors::build::EXCEPTION,
			"{} file '{}' does not match standard {} filename format",
			start, file_path.string(), start
		) };

		if (filename.size() < start.size() + 2 || filename.size() > start.size() + 3) {
			// Checking for stuff like ExGFX1.bin, ExGFX0001.bin, etc.
			throw verification_exception;
		}

		const auto number_part{ filename.substr(start.size(), 3) };
		std::string upper_number_part{ number_part };
		std::transform(upper_number_part.begin(), upper_number_part.end(), upper_number_part.begin(), ::toupper);
		if (number_part != upper_number_part) {
			// Checking for stuff like ExGFX1aB.bin
			throw verification_exception;
		}

		size_t parsed{};
		const auto number{ std::stoi(number_part, &parsed, 16) };

		if (parsed != 2 && parsed != 3) {
			throw verification_exception;
		}

		if (!exgfx) {
			if (parsed != 2 || number < 0 || number > 0x33) {
				throw verification_exception;
			}
		}
		else {
			if (parsed == 2) {
				if (number < 0x60 || (number > 0x63 && number < 0x80)) {
					throw verification_exception;
				}
			}
			else {
				if (number < 0x100) {
					throw verification_exception;
				}
			}
		}
	}

	void GraphicsUtil::importProjectGraphicsInto(const Configuration& config, const fs::path& rom_path) {
		importResources(config, rom_path, false);
	}

	void GraphicsUtil::importProjectExGraphicsInto(const Configuration& config, const fs::path& rom_path) {
		importResources(config, rom_path, true);
	}

	void GraphicsUtil::exportProjectGraphicsFrom(const Configuration& config, const fs::path& rom_path, bool keep_symlink) {
		exportResources(config, false);
	}

	void GraphicsUtil::exportProjectExGraphicsFrom(const Configuration& config, const fs::path& rom_path, bool keep_symlink) {
		exportResources(config, true);
	}

	void GraphicsUtil::linkOutputRomToProjectGraphics(const Configuration& config, bool exgfx) {
		const auto& target{ exgfx ? config.ex_graphics : config.graphics };
		if (!target.isSet()) {
			return;
		}

		const auto lunar_magic_export_folder{ getLunarMagicFolderPath(config.output_rom.getOrThrow(), exgfx) };
		const auto project_folder{ target.getOrThrow() };

		if (lunar_magic_export_folder != project_folder) {
			createSymlink(lunar_magic_export_folder, project_folder);
		}
	}
}
