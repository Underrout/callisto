#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <chrono>

#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "../not_found_exception.h"
#include "../dependency/policy.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace stardust {
	class ResourceDependency {
	public:
		const fs::path dependent_path;
		const std::optional<uint64_t> last_write_time;
		const Policy policy;

		ResourceDependency(const fs::path& dependent_path) : ResourceDependency(dependent_path, Policy::REINSERT) {}

		ResourceDependency(const fs::path& dependent_path, Policy policy)
			: dependent_path(dependent_path), policy(policy),
			last_write_time(fs::exists(dependent_path) ? 
				std::make_optional(fs::last_write_time(dependent_path).time_since_epoch().count()) : std::nullopt)
		{
			spdlog::debug(fmt::format("Resource dependency created on '{}' -> {}", 
				dependent_path.string(), 
				fs::exists(dependent_path) ? "exists" : "missing"
			));
		}

		ResourceDependency(const json& j) : dependent_path(j["path"].get<std::string>()), policy(j["policy"]), 
			last_write_time(j["timestamp"].is_null() ? std::nullopt : std::make_optional(j["timestamp"].get<uint64_t>())) {}

		json toJson() const {
			json j;
			j["path"] = dependent_path.string();
			j["policy"] = policy;
			if (last_write_time.has_value()) {
				j["timestamp"] = last_write_time.value();
			}
			else {
				j["timestamp"] = nullptr;
			}
			return j;
		}

		bool operator==(const ResourceDependency& other) const {
			return dependent_path == other.dependent_path && last_write_time == other.last_write_time;
		}
	};
}

namespace std {
	template<>
	struct hash<stardust::ResourceDependency> {
		size_t operator()(const stardust::ResourceDependency& dependency) const {
			return hash<std::string>{}(dependency.dependent_path.string());
		}
	};
}
