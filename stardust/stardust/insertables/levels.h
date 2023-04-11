#pragma once

#include <filesystem>
#include <optional>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

#include "../configuration/configuration.h"
#include "../dependency/policy.h"

namespace fs = std::filesystem;

namespace stardust {
	class Levels : public LunarMagicInsertable {
	protected:
		const fs::path levels_folder;
		std::optional<std::string> import_flag;

		std::unordered_set<ResourceDependency> determineDependencies() override;

	public:
		void insert() override;

		Levels(const Configuration& config);
	};
}
