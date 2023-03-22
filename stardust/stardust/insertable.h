#pragma once

#include <string>
#include <unordered_set>

#include "dependency/resource_dependency.h"
#include "dependency/configuration_dependency.h"
#include "configuration/config_variable.h"

namespace stardust {
	class Insertable {
	protected:
		std::unordered_set<ConfigurationDependency> configuration_dependencies{};

		virtual std::unordered_set<ResourceDependency> determineDependencies() = 0;

		static std::unordered_set<ResourceDependency> extractDependenciesFromReport(const fs::path& dependency_report_file_path) {
			if (!fs::exists(dependency_report_file_path)) {
				return {};
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

		const StringConfigVariable& registerConfigurationDependency(const StringConfigVariable& config_variable,
			ConfigurationDependency::Policy policy = ConfigurationDependency::Policy::REBUILD) {
			if (config_variable.isSet()) {
				std::variant<std::monostate, std::string, bool> variant;
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

		const PathConfigVariable& registerConfigurationDependency(const PathConfigVariable& config_variable,
			ConfigurationDependency::Policy policy = ConfigurationDependency::Policy::REBUILD) {
			if (config_variable.isSet()) {
				std::variant<std::monostate, std::string, bool> variant;
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

		const BoolConfigVariable& registerConfigurationDependency(const BoolConfigVariable& config_variable,
			ConfigurationDependency::Policy policy = ConfigurationDependency::Policy::REBUILD) {
			if (config_variable.isSet()) {
				std::variant<std::monostate, std::string, bool> variant;
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

	public:
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
