#pragma once

#include <map>
#include <optional>
#include <memory>
#include <numeric>
#include <filesystem>
#include <functional>
#include <iostream>
#include <sstream>

#include <toml.hpp>
#include <fmt/core.h>

#include "configuration_level.h"
#include "config_exception.h"

namespace fs = std::filesystem;

namespace stardust {
	template<typename T, typename V>
	class ConfigVariable {
	protected:
		const std::string name;
		const std::vector<std::string> keys;
		std::unordered_map<ConfigurationLevel, V> values{};

		std::string formatUserVariables(const std::string& str, 
			const std::map<std::string, std::string>& user_variables) const {
			
			std::ostringstream out{};
			std::ostringstream variable_name{};

			bool parsing_variable_name{ false };

			size_t i{ 0 };
			while (i < str.size()) {
				const auto character{ str.at(i) };

				if (character == '{') {
					if (i == str.size() - 1) {
						throw UserVariableException("Unclosed user variable usage",
							{ "Close the user variable usage, i.e. instead of '{user_variable' write '{user_variable}'" }, 
							"Braces around user variable name must be closed"
						);
					}
					const auto next_character{ str.at(i + 1) };

					if (next_character == '{') {
						if (parsing_variable_name) {
							throw UserVariableException("Malformed user variable usage",
								{ "To escape the '{' and '}' characters, use '{{' and '}}' respectively." }, 
								"Cannot use '{' character inside user variable usage"
							);
						}
						else {
							out << '{';
							++i;
						}
					}
					else {
						if (parsing_variable_name) {
							throw UserVariableException("Nested user variable usage is not allowed",
								{ "'{user_variable{user_variable}}' and similar are not valid, did you mean '{user_variable}{user_variable}'?" },
								"Cannot nest user variable usages inside each other"
							);
						}
						else {
							parsing_variable_name = true;
						}
					}
				}
				else if (character == '}') {
					if (parsing_variable_name) {
						parsing_variable_name = false;
						const std::string name{ variable_name.str() };

						if (user_variables.count(name) == 0) {
							throw UserVariableException(fmt::format(
								"User variable '{}' not found",
								name
							),
								{
									"User variables must be specified in a '[variables]' table",
									"User variables are case-sensitive",
									"To escape the '{' and '}' characters, use '{{' and '}}' respectively."
								},
								fmt::format("'{}' does not exist", name)
							);
						}

						out << user_variables.at(name);
						variable_name = std::ostringstream();
					}
					else {
						if (i != str.size() - 1 && str.at(i + 1) == '}') {
							out << '}';
							++i;
						}
						else {
							throw UserVariableException("Malformed user variable usage",
								{ "To escape the '{' and '}' characters, use '{{' and '}}' respectively." },
								"Invalid '}' character"
							);
						}
					}
				}
				else {
					if (parsing_variable_name) {
						variable_name << character;
					}
					else {
						out << character;
					}
				}
				++i;
			}

			if (parsing_variable_name) {
				throw UserVariableException("Unclosed user variable usage",
					{ "Close the user variable usage, i.e. instead of '{user_variable' write '{user_variable}'" },
					"Braces around user variable name must be closed"
				);
			}

			return out.str();
		}

		std::optional<T> getTomlValue(toml::value& table) const {
			auto value{ std::ref(table) };

			try {
				for (const auto& key : keys) {
					value = std::ref(toml::find(value.get(), key));
				}
			}
			catch (const std::out_of_range&) {
				// at least one of the keys was not present
				return std::nullopt;
			}

			return toml::get<T>(value.get());
		}

		void checkNotSet(ConfigurationLevel level) const {
			if (values.count(level) != 0) {
				throw DuplicateConfigVariableException(fmt::format(
					"Configuration variable '{}' specified multiple times",
					name
				));
			}
		}

	public:
		V getOrThrow() const {
			if (values.empty()) {
				throw MissingConfigVariableException(fmt::format(
					"Configuration variable '{}' not specified",
					name
				));
			}

			// getting value of largest key, meaning the highest level, i.e. project settings will override user settings, etc. 
			return std::max_element(values.begin(), values.end(), 
				[](const auto& v1, const auto& v2) { return v1.first < v2.first; })->second;
		}

		bool isSet() const {
			return !values.empty();
		}

		ConfigVariable(const std::vector<std::string>& keys) : keys(keys), 
			name(std::accumulate(keys.begin(), keys.end(), std::string("."))) {}
	};

	class PathConfigVariable : public ConfigVariable<toml::string, fs::path> {
	public:
		void trySet(toml::value& table, ConfigurationLevel level, const fs::path& relative_to, 
			const std::map<std::string, std::string>& user_variables);

		using ConfigVariable::ConfigVariable;
	};

	class StringConfigVariable : public ConfigVariable<toml::string, std::string> {
	public:
		void trySet(toml::value& table, ConfigurationLevel level,
			const std::map<std::string, std::string>& user_variables);

		using ConfigVariable::ConfigVariable;
	};

	class IntegerConfigVariable : public ConfigVariable<toml::integer, std::int16_t> {
	public:
		void trySet(toml::value& table, ConfigurationLevel level);

		using ConfigVariable::ConfigVariable;
	};
}
