#pragma once

#include <filesystem>
#include <algorithm>
#include <execution>

#include <fmt/core.h>
#include <boost/process.hpp>

#ifdef _WIN32
#include "junction/libntfslinks/include/Junction.h"
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

	public:
		static fs::path getExportFolderPath(const Configuration& config, bool exgfx) {
			const auto& project_folder{ exgfx ? config.ex_graphics : config.graphics };
			if (project_folder.isSet()) {
				return project_folder.getOrThrow();
			}
			return config.output_rom.getOrThrow().parent_path() / getLunarMagicFolderName(exgfx);
		}

		static void createSymlink(const fs::path& link_name, const fs::path& target_name) {
#ifndef _WIN32
			try {
				if (fs::exists(link_name)) {
					fs::remove_all(link_name);
				}
				fs::create_directory_symlink(target_name, link_name);
			}
			catch (const std::exception&) {
				throw CallistoException(fmt::format("Failed to create symlink {} -> {}", link_name.string(), target_name.string()));
			}
#else
			try {
				if (fs::exists(link_name)) {
					libntfslinks::DeleteJunction(link_name.string().c_str());
				}
				libntfslinks::CreateJunction(link_name.string().c_str(), target_name.string().c_str());
			}
			catch (const std::exception&) {
				throw CallistoException(fmt::format("Failed to create junction {} -> {}", link_name.string(), target_name.string()));
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
