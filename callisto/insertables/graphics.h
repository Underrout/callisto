#pragma once

#include <filesystem>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

#include "../graphics_util.h"

#include "../configuration/configuration.h"
#include "../dependency/policy.h" 

namespace fs = std::filesystem; 

namespace callisto {
	class Graphics : public LunarMagicInsertable {
	protected:
		const fs::path project_graphics_folder_path;
		const Configuration& config;

		std::unordered_set<ResourceDependency> determineDependencies() override;

	public:
		void insert() override;

		Graphics(const Configuration& config);
	};
}
