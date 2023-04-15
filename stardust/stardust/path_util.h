#pragma once

#include <filesystem>

#include <platform_folders.h>

namespace fs = std::filesystem;

namespace stardust {
	class PathUtil {
	private:
		static constexpr auto STARDUST_DIRECTORY_NAME{ ".stardust" };
		static constexpr auto CACHE_DIRECTORY_NAME{ ".cache" };
		static constexpr auto GLOBULES_IMPRINT_DIRECTORY_NAME{ "globules" };
		static constexpr auto INSERTED_GLOBULES_DIRECTORY_NAME{ "inserted_globules" };
		static constexpr auto BUILD_REPORT_FILE_NAME{ "build_report.json" };
		static constexpr auto GLOBULE_CALL_FILE{ "call.asm" };
		static constexpr auto ASSEMBLY_INFO_FILE{ "info.asm" };
		static constexpr auto USER_SETTINGS_FOLDER_NAME{ "stardust" };
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
 
		static fs::path getStardustDirectoryPath(const fs::path& project_root) {
			return project_root / STARDUST_DIRECTORY_NAME;
		}

		static fs::path getStardustCachePath(const fs::path& project_root) {
			return getStardustDirectoryPath(project_root) / CACHE_DIRECTORY_NAME;
		}

		static fs::path getBuildReportPath(const fs::path& project_root) {
			return getStardustCachePath(project_root) / BUILD_REPORT_FILE_NAME;
		}

		static fs::path getGlobuleImprintDirectoryPath(const fs::path& project_root) {
			return getStardustDirectoryPath(project_root) / GLOBULES_IMPRINT_DIRECTORY_NAME;
		}

		static fs::path getInsertedGlobulesDirectoryPath(const fs::path& project_root) {
			return getStardustCachePath(project_root) / INSERTED_GLOBULES_DIRECTORY_NAME;
		}

		static fs::path getGlobuleCallFilePath(const fs::path& project_root) {
			return getStardustCachePath(project_root) / GLOBULE_CALL_FILE;
		}

		static fs::path getAssemblyInfoFilePath(const fs::path& project_root) {
			return getStardustDirectoryPath(project_root) / ASSEMBLY_INFO_FILE;
		}
	};
}
