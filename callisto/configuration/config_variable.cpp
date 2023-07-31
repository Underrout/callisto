#include "config_variable.h"

namespace callisto {
	bool PathConfigVariable::trySet(const toml::value& table, ConfigurationLevel level, const PathConfigVariable& relative_to,
		const std::map<std::string, std::string>& user_variables) {

		const auto toml_value{ getTomlValue(table) };

		if (toml_value.has_value()) {
			checkNotSet(level, toml_value.value());
			const auto formatted{ formatUserVariables(toml_value.value(), user_variables) };;
			const auto full_path{ PathUtil::normalize(formatted, relative_to.getOrThrow()) };
			spdlog::debug(fmt::format(
				"Setting {} = {}",
				name,
				full_path.string()
			));
			values.insert({ level, full_path });
			return true;
		}
		return false;
	}

	bool PathConfigVariable::trySet(const toml::value& table, ConfigurationLevel level, const fs::path& relative_to,
		const std::map<std::string, std::string>& user_variables) {

		const auto toml_value{ getTomlValue(table) };

		if (toml_value.has_value()) {
			checkNotSet(level, toml_value.value());
			const auto formatted{ formatUserVariables(toml_value.value(), user_variables) };
			const auto full_path{ PathUtil::normalize(formatted, relative_to) };
			spdlog::debug(fmt::format(
				"Setting {} = {}",
				name,
				full_path.string()
			));
			values.insert({ level, full_path });
			return true;
		}
		return false;
	}

	bool StringConfigVariable::trySet(const toml::value& table, ConfigurationLevel level,
		const std::map<std::string, std::string>& user_variables) {

		const auto toml_value{ getTomlValue(table) };

		if (toml_value.has_value()) {
			checkNotSet(level, toml_value.value());
			const auto formatted{ formatUserVariables(toml_value.value(), user_variables) };
			spdlog::debug(fmt::format(
				"Setting {} = {}",
				name,
				formatted
			));
			values.insert({ level, formatted });
			return true;
		}
		return false;
	}

	bool IntegerConfigVariable::trySet(const toml::value& table, ConfigurationLevel level) {
		auto toml_value{ getTomlValue(table) };

		if (toml_value.has_value()) {
			checkNotSet(level, toml_value.value());
			const auto as_int{ static_cast<std::int16_t>(toml::get<toml::integer>(toml_value.value())) };
			spdlog::debug(fmt::format(
				"Setting {} = {}",
				name,
				as_int
			));
			values.insert({ level, as_int });
			return true;
		}
		return false;
	}

	bool ExtendablePathVectorConfigVariable::trySet(const toml::value& table, ConfigurationLevel level,
		const PathConfigVariable& relative_to, const std::map<std::string, std::string>& user_variables) {
		const auto toml_array{ getTomlValue(table) };

		if (toml_array.has_value()) {
			checkNotSet(level, toml_array.value());
			
			std::vector<fs::path> converted{};
			std::vector<std::string> as_strings{};

			for (const auto& entry : toml::get<toml::array>(toml_array.value())) {
				const auto formatted{ formatUserVariables(entry, user_variables) };
				const auto full_path{ PathUtil::normalize(formatted, relative_to.getOrThrow()) };
				converted.push_back(full_path);
				as_strings.push_back(full_path.string());
			}

			spdlog::debug(fmt::format(
				"Setting {} = [{}]",
				name,
				fmt::join(as_strings, ", ")
			));
			values.insert({ level, converted });
			return true;
		}
		return false;
	}

	std::vector<fs::path> ExtendablePathVectorConfigVariable::getOrThrow() const {
		if (values.empty()) {
			throw MissingConfigVariableException(fmt::format(
				"Configuration variable '{}' not specified",
				name
			));
		}

		std::vector<fs::path> joined{};

		for (int i{ 0 }; static_cast<ConfigurationLevel>(i) != ConfigurationLevel::_COUNT; ++i) {
			const auto config_level{ static_cast<ConfigurationLevel>(i) };

			if (values.count(config_level) != 0) {
				for (const auto& entry : values.at(config_level)) {
					joined.push_back(entry);
				}
			}
		}

		return joined;
	}

	bool StringVectorConfigVariable::trySet(const toml::value& table, ConfigurationLevel level,
		const std::map<std::string, std::string>& user_variables) {

		const auto toml_array{ getTomlValue(table) };

		if (toml_array.has_value()) {
			checkNotSet(level, toml_array.value());

			std::vector<std::string> converted{};

			for (const auto& entry : toml::get<toml::array>(toml_array.value())) {
				const auto formatted{ formatUserVariables(entry, user_variables) };
				converted.push_back(formatted);
			}

			spdlog::debug(fmt::format(
				"Setting {} = [{}]",
				name,
				fmt::join(converted, ", ")
			));
			values.insert({ level, converted });
			return true;
		}
		return false;
	}
	bool BoolConfigVariable::trySet(const toml::value& table, ConfigurationLevel level) {
		auto toml_value{ getTomlValue(table) };

		if (toml_value.has_value()) {
			checkNotSet(level, toml_value.value());
			const auto as_boolean{ static_cast<bool>(toml::get<toml::boolean>(toml_value.value())) };
			spdlog::debug(fmt::format(
				"Setting {} = {}",
				name,
				as_boolean
			));
			values.insert({ level, as_boolean });
			return true;
		}
		return false;
	}

	bool StaticResourceDependencyConfigVariable::trySet(const toml::value& table, ConfigurationLevel level,
		const PathConfigVariable& relative_to, const std::map<std::string, std::string>& user_variables) {
		const auto toml_array{ getTomlValue(table) };

		if (toml_array.has_value()) {
			checkNotSet(level, toml_array.value());

			std::vector<ResourceDependency> converted{};

			for (const auto& entry : toml::get<toml::array>(toml_array.value())) {
				const auto path{ toml::find<toml::string>(entry, "path") };
				Policy policy;

				if (entry.contains("policy")) {
					const auto policy_value{ toml::find(entry, "policy") };
					const auto as_string{ toml::get<std::string>(policy_value) };
					if (as_string == "rebuild") {
						policy = Policy::REBUILD;
					}
					else if (as_string == "reinsert") {
						policy = Policy::REINSERT;
					}
					else {
						throw TomlException(
							policy_value,
							"Unknown policy type",
							{
								"Only 'rebuild' or 'reinsert' are allowed as policies"
							},
							fmt::format("'{}' is not allowed here", as_string)
						);
					}
				}
				else {
					policy = Policy::REINSERT;
				}

				const auto formatted{ formatUserVariables(path, user_variables) };
				const auto full_path{ PathUtil::normalize(formatted, relative_to.getOrThrow()) };

				const auto dependency{ ResourceDependency(full_path, policy) };
				converted.push_back(dependency);

				if (fs::is_directory(full_path)) {
					for (const auto& entry : fs::recursive_directory_iterator(full_path)) {
						const auto dependency{ ResourceDependency(entry.path(), policy)};
						converted.push_back(dependency);
					}
				}
			}

			values.insert({ level, converted });
			return true;
		}
		return false;
	}
}
