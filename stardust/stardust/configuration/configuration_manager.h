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

namespace stardust {
	class ConfigurationManager {
	private:
		static constexpr auto VARIABLE_FILE_NAME{ "variables.toml" };
		static constexpr auto PROFILE_FOLDER_NAME{ "profiles" };
		static constexpr auto USER_SETTINGS_FOLDER_NAME{ "stardust" };

		const fs::path stardust_root;

	public:
		ConfigurationManager(const fs::path& stardust_directory);

		std::vector<std::string> getProfileNames() const;
		std::shared_ptr<Configuration> getConfiguration(std::optional<std::string> current_profile) const;
	};
}
