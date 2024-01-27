#pragma once

#include <filesystem>
#include <algorithm>
#include <execution>

#include <fmt/core.h>
#include <boost/process.hpp>

#ifdef _WIN32
#include "junction/libntfslinks/include/Junction.h"
#include "windows.h"
#endif

#include <spdlog/spdlog.h>

#include "insertion_exception.h"
#include "extractables/extraction_exception.h"
#include "not_found_exception.h"

#include "configuration/configuration.h"

#include "prompt_util.h"

#include "colors.h"

namespace fs = std::filesystem;
namespace bp = boost::process;

namespace callisto {
	class GraphicsUtil {
	protected:
		class VerificationException : public CallistoException {
		public:
			using CallistoException::CallistoException;
		};

		static constexpr auto GRAPHICS_FOLDER_NAME{ "Graphics" };
		static constexpr auto EX_GRAPHICS_FOLDER_NAME{ "ExGraphics" };

		static constexpr auto GRAPHICS_EXPORT_COMMAND{ "-ExportGFX" };
		static constexpr auto EX_GRAPHICS_EXPORT_COMMAND{ "-ExportExGFX" };

		static constexpr auto GRAPHICS_IMPORT_COMMAND{ "-ImportGFX" };
		static constexpr auto EX_GRAPHICS_IMPORT_COMMAND{ "-ImportExGFX" };

		static void exportResources(const Configuration& config, bool exgfx);
		static void importResources(const Configuration& config, const fs::path& rom_path, bool exgfx);

		static void fixPotentialExportDiscrepancy(const fs::path& new_folder, const fs::path& old_folder, bool exgfx, bool allow_user_input);

		static bool oldAndNewDiffer(const fs::path& new_folder, const fs::path& old_folder, bool exgfx);

		static void verifyFilenames(const fs::path& graphics_folder, bool exgfx);
		static void verifyFilename(const fs::path& file_path, bool exgfx);

		static bool filesEqual(const fs::path& file1, const fs::path& file2);

		static inline std::string getImportCommand(bool exgfx) {
			return exgfx ? EX_GRAPHICS_IMPORT_COMMAND : GRAPHICS_IMPORT_COMMAND;
		}

		static inline std::string getExportCommand(bool exgfx) {
			return exgfx ? EX_GRAPHICS_EXPORT_COMMAND : GRAPHICS_EXPORT_COMMAND;
		}

		static inline std::string getLunarMagicFolderName(bool exgfx) {
			return exgfx ? EX_GRAPHICS_FOLDER_NAME : GRAPHICS_FOLDER_NAME;
		}

		static void deleteSymlink(const fs::path& symlink_path) {
#ifndef _WIN32
			try {
				fs::remove(symlink_path);
			}
			catch (const std::exception&) {
				throw CallistoException(fmt::format("Failed to delete symlink {}", symlink_path.string()));
			}
#else
			try {
				libntfslinks::DeleteJunction(symlink_path.string().c_str());
			}
			catch (const std::exception&) {
				throw CallistoException(fmt::format("Failed to delete junction {}", symlink_path.string()));
			}
#endif
		}

		template<typename... Args>
		static inline int callLunarMagic(const Configuration& config, Args... args) {
			return bp::system(config.lunar_magic_path.getOrThrow().string(), args...);
		}

		static void moveWithFallback(const fs::path& source, const fs::path& target) {
			try {
				fs::remove_all(target);
				fs::rename(source, target);
			}
			catch (const fs::filesystem_error &e) {
				spdlog::warn(fmt::format(colors::WARNING,
					"Failed to move source folder '{}' to target folder '{}' via renaming, "
					"you might have a file in '{}' open in another program. Falling back to element-wise overwrite. "
					"Underlying filesystem error was:\n\r{}",
					source.string(), target.string(), target.string(),
					e.what()
				));

				try {
					for (const auto& entry : fs::directory_iterator(source)) {
						const auto destination{ fs::path(target) / entry.path().filename().string() };
						fs::copy_file(entry.path(), destination, fs::copy_options::overwrite_existing);
					}

					fs::remove_all(source);
					spdlog::debug("Successfully performed element-wise overwrite.");
				}
				catch (const fs::filesystem_error& ie) {
					throw CallistoException(fmt::format("Failed to perform element-wise overwrite with underlying exception:\n\r{}", ie.what()));
				}
			}
		}

#ifdef _WIN32
		static bool canUseJunction(const fs::path& link_name, const fs::path& target_name) {
			if (link_name.root_name() != target_name.root_name()) {
				return false;
			}

			const std::string root{ target_name.root_name().string() + '\\' };
			DWORD fsFlags;
			GetVolumeInformation(
				root.data(),
				NULL, 0, NULL, NULL, &fsFlags, NULL, 0
			);

			return fsFlags & FILE_SUPPORTS_REPARSE_POINTS;
		}
#endif

		static void _create_symlink(const fs::path& link_name, const fs::path& target_name) {
			if (fs::exists(link_name)) {
				fs::remove(link_name);
			}
			fs::create_directory_symlink(target_name, link_name);
		}

	public:
		static fs::path getExportFolderPath(const Configuration& config, bool exgfx) {
			const auto& project_folder{ exgfx ? config.ex_graphics : config.graphics };
			if (project_folder.isSet()) {
				return project_folder.getOrThrow();
			}
			return config.output_rom.getOrThrow().parent_path() / getLunarMagicFolderName(exgfx);
		}

		static void ensureUsableDestination(const fs::path& destination, const fs::path& source) {
			try {
				createSymlink(destination, source);
			}
			catch (const std::exception& e) {
				spdlog::warn(fmt::format(colors::WARNING,
					"Failed to create symbolic link {} -> {}\n\r"
					"Using hard copies instead, this can slow down build times\n\r"
					"Underlying exception:\n\r{}",
					destination.string(), source.string(),
					e.what()
				));
				
				try {
					fs::copy(source, destination, fs::copy_options::overwrite_existing);
				}
				catch (const std::exception& e) {
					throw CallistoException(fmt::format("Failed to create hard copy of folder {} at {}", source.string(), destination.string()));
				}
			}
		}

		static void cleanUpDestination(const fs::path& destination) {
			try {
				if (!fs::exists(destination)) {
					return;
				}

				if (fs::is_symlink(destination)) {
					fs::remove(destination);
					return;
				}

				if (fs::is_directory(destination)) {
					fs::remove_all(destination);
					return;
				}

#ifdef _WIN32
				libntfslinks::DeleteJunction(destination.string().c_str());
#endif
			}
			catch (const std::exception& e) {
				spdlog::warn(fmt::format(colors::WARNING, "Failed to clean up {}", destination.string()));
			}
		}

		static void createSymlink(const fs::path& link_name, const fs::path& target_name) {
#ifndef _WIN32
			_create_symlink(link_name, target_name);
#else
			if (canUseJunction(link_name, target_name)) {
				try {
					if (fs::exists(link_name)) {
						libntfslinks::DeleteJunction(link_name.string().c_str());
					}
					libntfslinks::CreateJunction(link_name.string().c_str(), target_name.string().c_str());
				}
				catch (const std::exception&) {
					_create_symlink(link_name, target_name);
				}
			}
			else {
				_create_symlink(link_name, target_name);
			}
#endif
		}

		static inline fs::path getLunarMagicFolderPath(const fs::path& target_rom_path, bool exgfx) {
			return target_rom_path.parent_path() / getLunarMagicFolderName(exgfx);
		}

		static void importProjectGraphicsInto(const Configuration& config, const fs::path& rom_path);
		static void importProjectExGraphicsInto(const Configuration& config, const fs::path& rom_path);

		static void exportProjectGraphicsFrom(const Configuration& config, const fs::path& rom_path, bool keep_symlink = true);
		static void exportProjectExGraphicsFrom(const Configuration& config, const fs::path& rom_path, bool keep_symlink = true);

		static void linkOutputRomToProjectGraphics(const Configuration& config, bool exgfx);
	};
}
