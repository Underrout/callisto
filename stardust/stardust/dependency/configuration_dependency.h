#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <chrono>
#include <variant>

#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "../insertable.h"
#include "../not_found_exception.h"
#include "policy.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace stardust {
	class ConfigurationDependency {
	protected:
		std::variant<std::monostate, std::string, bool> getValue(const json& j) {
			if (j["value"].is_null()) {
				return {};
			}
			else {
				if (j["value"].is_string()) {
					return { j["value"].get<std::string>() };
				}
				else if (j["value"].is_boolean()) {
					return { j["value"].get<bool>() };
				}
			}
		}

	public:
		const std::string config_keys;
		const std::variant<std::monostate, std::string, bool> value;
		const Policy policy;

		ConfigurationDependency(const std::string& config_keys, 
		const std::variant<std::monostate, std::string, bool> value, Policy policy)
			: config_keys(config_keys), value(value), policy(policy) {}

		ConfigurationDependency(const json& j) 
			: config_keys(j["config_keys"]), value(getValue(j)), policy(j["policy"]) {}

		json toJson() const {
			json j;
			j["config_keys"] = config_keys;

			if (!std::holds_alternative<std::monostate>(value)) {
				if (std::holds_alternative<std::string>(value)) {
					j["value"] = std::get<std::string>(value);
				}
				else {
					j["value"] = std::get<bool>(value);
				}
			}
			else {
				j["value"] = nullptr;
			}

			j["policy"] = policy;

			return j;
		}

		bool operator==(const ConfigurationDependency& other) const {
			return config_keys == other.config_keys;
		}
	};
}

namespace std {
	template<>
	struct hash<stardust::ConfigurationDependency> {
		size_t operator()(const stardust::ConfigurationDependency& dependency) const {
			return hash<std::string>{}(dependency.config_keys);
		}
	};
}

