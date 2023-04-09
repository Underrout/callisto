#pragma once

#include <filesystem>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

#include "../configuration/configuration.h"
#include "../dependency/policy.h"

namespace fs = std::filesystem;

namespace stardust {
	class Level : public LunarMagicInsertable {
	protected:
		const fs::path mwl_file;

		std::unordered_set<ResourceDependency> determineDependencies() override;

	public:
		void insert() override;

		Level(const Configuration& config, const fs::path& mwl_file);
	};
}
