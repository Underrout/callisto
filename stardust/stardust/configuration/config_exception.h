#pragma once

#include "../stardust_exception.h"

namespace stardust {
	class ConfigException : public StardustException {
		using StardustException::StardustException;
	};

	class DuplicateConfigVariableException : public ConfigException {
		using ConfigException::ConfigException;
	};

	class MissingConfigVariableException : public ConfigException {
		using ConfigException::ConfigException;
	};

	class UserVariableException : public ConfigException {
	public:
		const std::vector<std::string> hints{};
		const std::string comment{};

		UserVariableException(const std::string& message, const std::vector<std::string>& hints, const std::string& comment) : 
			ConfigException(message), hints(hints), comment(comment) {}
	};
}
