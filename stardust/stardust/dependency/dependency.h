#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>

#include <fmt/format.h>

#include "../insertable.h"
#include "../not_found_exception.h"

namespace fs = std::filesystem;

namespace stardust {
	class Dependency {
	public:
		const fs::path dependent_path;
		const std::optional<fs::file_time_type> last_write_time;

		Dependency(const fs::path& dependent_path)
			: dependent_path(dependent_path), 
			last_write_time(fs::exists(dependent_path) ? std::make_optional(fs::last_write_time(dependent_path)) : std::nullopt)
		{}

		bool operator==(const Dependency& other) const {
			return dependent_path == other.dependent_path;
		}

		static std::unordered_set<Dependency> fromDependencyReportFile(const fs::path& dependency_report_file_path) {
			if (!fs::exists(dependency_report_file_path)) {
				return {};
			}

			std::unordered_set<Dependency> dependencies{};

			std::ifstream dependency_file{ dependency_report_file_path };
			std::string line;
			while (std::getline(dependency_file, line)) {
				dependencies.insert(Dependency(line));
			}

			return dependencies;
		}
	};
}

namespace std {
	template<>
	struct hash<stardust::Dependency> {
		size_t operator()(const stardust::Dependency& dependency) const {
			return hash<stardust::Dependency>{}(dependency.dependent_path);
		}
	};
}
