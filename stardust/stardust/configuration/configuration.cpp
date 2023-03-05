#include "configuration.h"

namespace stardust {
	std::map<std::string, std::string> Configuration::parseUserVariables(const std::vector<toml::value>& tables,
		const std::map<std::string, std::string>& previous_user_variables) {

		std::map<std::string, std::string> user_variables{};
		std::map<std::string, const toml::value&> toml_values{};

		for (const auto& table : tables) {
			std::map<std::string, std::string> local_user_variables{};
			std::map<std::string, const toml::value&> local_toml_values{};

			for (const auto& [key, value] : toml::find<toml::table>(table, "variables")) {
				if (user_variables.count(key) != 0) {
					throw TomlException2(
						value,
						toml_values.at(key),
						"Duplicate user variable",
						{
							"The same user variable cannot be specified multiple times at the same level of configuration"
						},
						fmt::format("User variable '{}' specified multiple times", key),
						"Previously specified here"
					);
				}

				auto joined_vars{ previous_user_variables };

				joined_vars.insert(local_user_variables.begin(), local_user_variables.end());

				local_user_variables.insert({ 
					key, 
					ConfigVariable<toml::string, std::string>::formatUserVariables(value, joined_vars)
				});
				local_toml_values.insert({ key, value });
			}

			user_variables.insert(local_user_variables.begin(), local_user_variables.end());
			toml_values.insert(local_toml_values.begin(), local_toml_values.end());
		}

		return user_variables;
	}
}
