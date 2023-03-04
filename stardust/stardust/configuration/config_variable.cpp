#include "config_variable.h"

namespace stardust {
	void PathConfigVariable::trySet(toml::value& table, ConfigurationLevel level, const fs::path& relative_to, 
		const std::map<std::string, std::string>& user_variables) {
		checkNotSet(level);

		const auto toml_value{ getTomlValue(table) };

		if (toml_value.has_value()) {
			std::string formatted{ formatUserVariables(std::string(toml_value.value()), user_variables) };

			values.insert({ level, relative_to / fs::path(formatted) });
		}
	}

	void StringConfigVariable::trySet(toml::value& table, ConfigurationLevel level, 
		const std::map<std::string, std::string>& user_variables) {
		checkNotSet(level);

		const auto toml_value{ getTomlValue(table) };

		if (toml_value.has_value()) {
			std::string formatted{ formatUserVariables(std::string(toml_value.value()), user_variables) };

			values.insert({ level, formatted });
		}
	}


	void IntegerConfigVariable::trySet(toml::value& table, ConfigurationLevel level) {
		checkNotSet(level);

		auto toml_value{ getTomlValue(table) };

		if (toml_value.has_value()) {
			values.insert({ level, toml_value.value() });
		}
	}
}
