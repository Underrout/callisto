#pragma once

#include <filesystem>

#include <platform_folders.h>

namespace fs = std::filesystem;

namespace callisto {
	class PathUtil {
	private:
		static constexpr auto CALLISTO_DIRECTORY_NAME{ ".callisto" };
		static constexpr auto CACHE_DIRECTORY_NAME{ ".cache" };
		static constexpr auto GLOBULES_IMPRINT_DIRECTORY_NAME{ "globules" };
		static constexpr auto INSERTED_GLOBULES_DIRECTORY_NAME{ "inserted_globules" };
		static constexpr auto BUILD_REPORT_FILE_NAME{ "build_report.json" };
		static constexpr auto LAST_ROM_SYNC_TIME_FILE_NAME{ "last_rom_sync.json" };
		static constexpr auto GLOBULE_CALL_FILE{ "call.asm" };
		static constexpr auto ASSEMBLY_INFO_FILE{ "info.asm" };
		static constexpr auto USER_SETTINGS_FOLDER_NAME{ "callisto" };
		static constexpr auto RECENT_PROJECTS_FILE{ "recent_projects.json" };

	public:
		static fs::path normalize(const fs::path& path, const fs::path& relative_to) {
			if (path.is_absolute()) {
				return fs::weakly_canonical(path);
			} 

			return fs::absolute(fs::weakly_canonical(relative_to / path));
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

		static fs::path getGlobuleImprintDirectoryPath(const fs::path& project_root) {
			return getCallistoDirectoryPath(project_root) / GLOBULES_IMPRINT_DIRECTORY_NAME;
		}

		static fs::path getInsertedGlobulesDirectoryPath(const fs::path& project_root) {
			return getCallistoCachePath(project_root) / INSERTED_GLOBULES_DIRECTORY_NAME;
		}

		static fs::path getGlobuleCallFilePath(const fs::path& project_root) {
			return getCallistoCachePath(project_root) / GLOBULE_CALL_FILE;
		}

		static fs::path getAssemblyInfoFilePath(const fs::path& project_root) {
			return getCallistoDirectoryPath(project_root) / ASSEMBLY_INFO_FILE;
		}
	};
}
