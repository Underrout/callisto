#pragma once

#include <toml.hpp>

#include "../callisto_exception.h"

namespace callisto {
	class ConfigException : public CallistoException {
		using CallistoException::CallistoException;
	};

	class MissingConfigVariableException : public ConfigException {
		using ConfigException::ConfigException;
	};

	class TomlException : public ConfigException {
	public:
		const toml::value toml_value;
		const std::vector<std::string> hints{};
		const std::string comment{};

		TomlException(const toml::value& toml_value, const std::string& message, 
			const std::vector<std::string>& hints, const std::string& comment) :
			ConfigException(message), toml_value(toml_value), hints(hints), comment(comment) {}
	};

	class TomlException2 : public TomlException {
	public: 
		const toml::value toml_value2;
		const std::string comment2{};

		TomlException2(const toml::value& toml_value, const toml::value& toml_value2, const std::string& message,
			const std::vector<std::string>& hints, const std::string& comment, const std::string& comment2) :
			TomlException(toml_value, message, hints, comment), toml_value2(toml_value2), comment2(comment2) {}
	};
}
