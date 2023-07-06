#pragma once

#include <map>
#include <optional>
#include <memory>
#include <numeric>
#include <filesystem>
#include <functional>
#include <iostream>
#include <sstream>
#include <array>

#include <spdlog/spdlog.h>
#include <toml.hpp>
#include <fmt/core.h>

#include "configuration.h"
#include "configuration_level.h"
#include "config_exception.h"

namespace fs = std::filesystem;

namespace callisto {
	class ConfigurationManager {
	private:
		static constexpr auto VARIABLE_FILE_NAME{ "variables.toml" };
		static constexpr auto PROFILE_FOLDER_NAME{ "profiles" };

		const fs::path callisto_root;


	public:
		ConfigurationManager(const fs::path& callisto_directory);

		std::vector<std::string> getProfileNames() const;
		std::shared_ptr<Configuration> getConfiguration(std::optional<std::string> current_profile, bool allow_user_input = true) const;

		std::unordered_map<ConfigurationLevel, std::vector<fs::path>> getConfigFilesToParse(const std::optional<std::string>& profile);
	};
}
