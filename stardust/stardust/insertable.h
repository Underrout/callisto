#pragma once

#include <string>
#include <unordered_set>

#include "dependency/dependency.h"

namespace stardust {
	class Insertable {
	protected:
		virtual std::unordered_set<Dependency> determineDependencies() = 0;

		static std::unordered_set<Dependency> extractDependenciesFromReport(const fs::path& dependency_report_file_path) {
			if (!fs::exists(dependency_report_file_path)) {
				return {};
			}

			std::unordered_set<Dependency> dependencies{};

			std::ifstream dependency_file{ dependency_report_file_path };
			std::string line;
			while (std::getline(dependency_file, line)) {
				fs::path path{ line };
				if (!path.is_absolute()) {
					path = dependency_report_file_path.parent_path() / path;
				}
				dependencies.insert(Dependency(fs::absolute(fs::weakly_canonical(path))));
			}

			dependency_file.close();

			// delete the file since it's just clutter lying around otherwise
			fs::remove(dependency_report_file_path);

			return dependencies;
		}

	public:
		virtual void insert() = 0;

		std::unordered_set<Dependency> insertWithDependencies() {
			insert();
			return determineDependencies();
		}
	};
}
