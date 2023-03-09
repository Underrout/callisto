#include "config_variable.h"

namespace stardust {
	bool PathConfigVariable::trySet(const toml::value& table, ConfigurationLevel level, const PathConfigVariable& relative_to,
		const std::map<std::string, std::string>& user_variables) {

		const auto toml_value{ getTomlValue(table) };

		if (toml_value.has_value()) {
			checkNotSet(level, toml_value.value());
			const auto formatted{ formatUserVariables(toml_value.value(), user_variables) };
			const auto joined_path{ fs::path(formatted).is_absolute() ? formatted : relative_to.getOrThrow() / formatted };
			const auto full_path{ fs::absolute(fs::weakly_canonical(joined_path)) };
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
			const auto joined_path{ fs::path(formatted).is_absolute() ? formatted : relative_to / formatted };
			const auto full_path{ fs::absolute(fs::weakly_canonical(joined_path)) };
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
				const auto joined_path{ fs::path(formatted).is_absolute() ? formatted : relative_to.getOrThrow() / formatted };
				const auto full_path{ fs::absolute(fs::weakly_canonical(joined_path)) };
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
}