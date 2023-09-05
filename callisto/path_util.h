#pragma once

#include <filesystem>

#include <platform_folders.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>

namespace fs = std::filesystem;

namespace callisto {
	class PathUtil {
	private:
		static constexpr auto CALLISTO_DIRECTORY_NAME{ ".callisto" };

		static constexpr auto CACHE_DIRECTORY_NAME{ ".cache" };

		static constexpr auto MODULES_DIRECTORY_NAME{ "modules" };
		static constexpr auto MODULES_CLEANUP_DIRECTORY_NAME{ "cleanup" };
		static constexpr auto MODULES_OLD_DIRECTORY_NAME{ "old" };
		static constexpr auto MODULES_CURRENT_DIRECTORY_NAME{ "current" };

		static constexpr auto BUILD_REPORT_FILE_NAME{ "build_report.json" };
		static constexpr auto LAST_ROM_SYNC_TIME_FILE_NAME{ "last_rom_sync.json" };
		static constexpr auto ASSEMBLY_INFO_FILE{ "callisto.asm" };
		static constexpr auto USER_SETTINGS_FOLDER_NAME{ "callisto" };
		static constexpr auto RECENT_PROJECTS_FILE{ "recent_projects.json" };
		static constexpr auto TEMPORARY_SUFFIX{ "_temp" };
		static constexpr auto TEMPORARY_RESOURCES_FOLDER_NAME{ "resources" };

		static constexpr auto CALLISTO_TEMP_FOLDER_NAME{ "callisto_build_temp" };

	public:
		static fs::path normalize(const fs::path& path, const fs::path& relative_to) {
			if (path.is_absolute()) {
				return fs::weakly_canonical(path);
			} 

			return fs::absolute(fs::weakly_canonical(relative_to / path));
		}

		static fs::path sanitizeForAsar(const fs::path& path) {
			auto as_string{ path.string() };
			boost::replace_all(as_string, "!", "\\!");
			boost::replace_all(as_string, "\"", "\"\"");

			return as_string;
		}

		static fs::path getUserSettingsPath() {
			return fs::path(sago::getConfigHome()) / USER_SETTINGS_FOLDER_NAME;
		}

		static fs::path getUserWideCachePath() {
			return getUserSettingsPath() / CACHE_DIRECTORY_NAME;
		}
		
		static fs::path getRecentProjectsPath() {
			return getUserWideCachePath() / RECENT_PROJECTS_FILE;
		}
 
		static fs::path getCallistoDirectoryPath(const fs::path& project_root) {
			return project_root / CALLISTO_DIRECTORY_NAME;
		}

		static fs::path getCallistoCachePath(const fs::path& project_root) {
			return getCallistoDirectoryPath(project_root) / CACHE_DIRECTORY_NAME;
		}

		static fs::path getBuildReportPath(const fs::path& project_root) {
			return getCallistoCachePath(project_root) / BUILD_REPORT_FILE_NAME;
		}

		static fs::path getLastRomSyncPath(const fs::path& project_root) {
			return getCallistoCachePath(project_root) / LAST_ROM_SYNC_TIME_FILE_NAME;
		}

		static fs::path getModuleCacheDirectoryPath(const fs::path& project_root) {
			return getCallistoCachePath(project_root) / MODULES_DIRECTORY_NAME;
		}

		static fs::path getModuleOldSymbolsDirectoryPath(const fs::path& project_root) {
			return getModuleCacheDirectoryPath(project_root) / MODULES_OLD_DIRECTORY_NAME;
		}

		static fs::path getModuleCleanupDirectoryPath(const fs::path& project_root) {
			return getModuleCacheDirectoryPath(project_root) / MODULES_CLEANUP_DIRECTORY_NAME;
		}

		static fs::path getUserModuleDirectoryPath(const fs::path& project_root) {
			return getCallistoDirectoryPath(project_root) / MODULES_DIRECTORY_NAME;
		}

		static fs::path getCallistoAsmFilePath(const fs::path& project_root) {
			return getCallistoDirectoryPath(project_root) / ASSEMBLY_INFO_FILE;
		}

		static fs::path getTemporaryFolderPath() {
			const auto path{ fs::temp_directory_path() / CALLISTO_TEMP_FOLDER_NAME };
			if (fs::exists(path)) {
				fs::remove_all(path);
			}
			fs::create_directory(path);
			return path;
		}

		static fs::path getTemporaryRomPath(const fs::path& temporary_folder_path, const fs::path& output_rom_path) {
			fs::create_directories(temporary_folder_path);
			return temporary_folder_path / fs::path(output_rom_path.stem().string() + TEMPORARY_SUFFIX + output_rom_path.extension().string());
		}

		static fs::path getTemporaryResourcePathFor(const fs::path& temporary_folder_path, const fs::path& resource_name) {
			const auto resource_folder_path{ temporary_folder_path / TEMPORARY_RESOURCES_FOLDER_NAME };
			fs::create_directories(resource_folder_path);
			return resource_folder_path / resource_name;
		}

		static inline fs::path convertToPosixPath(const fs::path& path) {
#ifndef _WIN32
			return path;
#else
			auto str_path{ path.string() };
			std::replace(str_path.begin(), str_path.end(), '\\', '/');
			return fs::path(str_path);
#endif
		}
	};
}
