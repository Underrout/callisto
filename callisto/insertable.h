#pragma once

#include <string>
#include <unordered_set>

#include "dependency/configuration_dependency.h"
#include "configuration/config_variable.h"
#include "dependency/policy.h"
#include "dependency/resource_dependency.h"

#include "colors.h"

namespace callisto {
	class Insertable {
	public:
		class NoDependencyReportFound : public CallistoException {
			using CallistoException::CallistoException;
		};
	protected:
		std::unordered_set<ConfigurationDependency> configuration_dependencies{};

		virtual std::unordered_set<ResourceDependency> determineDependencies() = 0;

		static std::unordered_set<ResourceDependency> extractDependenciesFromReport(const fs::path& dependency_report_file_path) {
			if (!fs::exists(dependency_report_file_path)) {
				throw NoDependencyReportFound(fmt::format(
					colors::NOTIFICATION,
					"No dependency report found at {}",
					dependency_report_file_path.string()
				));
			}

			std::unordered_set<ResourceDependency> dependencies{};

			std::ifstream dependency_file{ dependency_report_file_path };
			std::string line;
			while (std::getline(dependency_file, line)) {
				fs::path path{ line };
				if (!path.is_absolute()) {
					path = dependency_report_file_path.parent_path() / path;
				}
				dependencies.insert(ResourceDependency(fs::absolute(fs::weakly_canonical(path))));
			}

			dependency_file.close();

			// delete the file since it's just clutter lying around otherwise
			fs::remove(dependency_report_file_path);

			return dependencies;
		}

		template<typename T, typename V>
		const ConfigVariable<T, V>& registerConfigurationDependency(const ConfigVariable<T, V>& config_variable, Policy policy = Policy::REBUILD) {
			if (config_variable.isSet()) {
				std::variant<std::monostate, std::string, bool, std::vector<fs::path>> variant;
				if (config_variable.isSet()) {
					variant = config_variable.getOrThrow();
				}
				else {
					variant = std::monostate();
				}
				configuration_dependencies.insert(
					ConfigurationDependency(
					config_variable.name, variant,
					policy
				));
			}
			return config_variable;
		}

		template<typename T>
		const ConfigVariable<T, fs::path>& registerConfigurationDependency(const ConfigVariable<T, fs::path>& config_variable, Policy policy = Policy::REBUILD) {
			if (config_variable.isSet()) {
				std::variant<std::monostate, std::string, bool, std::vector<fs::path>> variant;
				if (config_variable.isSet()) {
					variant = config_variable.getOrThrow().string();
				}
				else {
					variant = std::monostate();
				}
				configuration_dependencies.insert(
					ConfigurationDependency(
						config_variable.name, variant,
						policy
					));
			}
			return config_variable;
		}

		static std::vector<ResourceDependency> getResourceDependenciesFor(const fs::path& folder_or_file, Policy policy) {
			std::vector<ResourceDependency> dependencies{ ResourceDependency(folder_or_file, policy) };

			if (fs::is_directory(folder_or_file)) {
				for (const auto& entry : fs::recursive_directory_iterator(folder_or_file)) {
					dependencies.push_back(ResourceDependency(entry.path(), policy));
				}

			}
			return dependencies;
		}

	public:
		virtual void init() {}
		virtual void insert() = 0;

		std::unordered_set<ResourceDependency> insertWithDependencies() {
			insert();
			return determineDependencies();
		}

		std::unordered_set<ConfigurationDependency> getConfigurationDependencies() const {
			return configuration_dependencies;
		}

		virtual ~Insertable() {}
	};
}
