#include "config_variable.h"

namespace stardust {
	void PathConfigVariable::trySet(toml::value& table, ConfigurationLevel level, const fs::path& relative_to, 
		const std::map<std::string, std::string>& user_variables) {
		checkNotSet(level);

		const auto toml_value{ getTomlValue(table) };

		if (toml_value.has_value()) {
			std::string formatted{ formatUserVariablesLogFailure(
				toml::get<toml::string>(toml_value.value()), user_variables, toml_value.value())
			};

			values.insert({ level, fs::absolute(fs::weakly_canonical(relative_to / formatted)) });
		}
	}

	void StringConfigVariable::trySet(toml::value& table, ConfigurationLevel level, 
		const std::map<std::string, std::string>& user_variables) {
		checkNotSet(level);

		const auto toml_value{ getTomlValue(table) };

		if (toml_value.has_value()) {
			std::string formatted{ formatUserVariablesLogFailure(
				toml::get<toml::string>(toml_value.value()), user_variables, toml_value.value())
			};

			values.insert({ level, formatted });
		}
	}

	void IntegerConfigVariable::trySet(toml::value& table, ConfigurationLevel level) {
		checkNotSet(level);

		auto toml_value{ getTomlValue(table) };

		if (toml_value.has_value()) {
			values.insert({ level, static_cast<std::int16_t>(toml::get<toml::integer>(toml_value.value())) });
		}
	}

	void ExtendablePathVectorConfigVariable::trySet(toml::value& table, ConfigurationLevel level,
		const fs::path& relative_to, const std::map<std::string, std::string>& user_variables) {
		checkNotSet(level);

		const auto toml_array{ getTomlValue(table) };

		if (toml_array.has_value()) {
			std::vector<fs::path> converted{};

			for (const auto& entry : toml::get<toml::array>(toml_array.value())) {
				const auto& value{ toml::get<toml::string>(entry) };
				const auto formatted{ formatUserVariablesLogFailure(value, user_variables, entry) };

				const auto full_path{ fs::absolute(fs::weakly_canonical(relative_to / formatted)) };

				converted.push_back(full_path);
			}

			values.insert({ level, converted });
		}
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

	void StringVectorConfigVariable::trySet(toml::value& table, ConfigurationLevel level, 
		const std::map<std::string, std::string>& user_variables) {
		checkNotSet(level);

		const auto toml_array{ getTomlValue(table) };

		if (toml_array.has_value()) {
			std::vector<std::string> converted{};

			for (const auto& entry : toml::get<toml::array>(toml_array.value())) {
				const auto& value{ toml::get<toml::string>(entry) };

				converted.push_back(formatUserVariablesLogFailure(value, user_variables, entry));
			}

			values.insert({ level, converted });
		}
	}
}
