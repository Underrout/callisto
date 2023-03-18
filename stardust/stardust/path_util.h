#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace stardust {
	class PathUtil {
	private:
		static constexpr auto CACHE_DIRECTORY_NAME{ ".stardust" };

	public:
		static fs::path normalize(const fs::path& path, const fs::path& relative_to) {
			if (path.is_absolute()) {
				return fs::weakly_canonical(path);
			} 

			return fs::absolute(fs::weakly_canonical(relative_to / path));
		}

		static fs::path getStardustCache(const fs::path& project_root) {
			return project_root / CACHE_DIRECTORY_NAME;
		}
	};
}
