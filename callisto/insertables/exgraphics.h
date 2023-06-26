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
	class ExGraphics : public LunarMagicInsertable {
	protected:
		const fs::path project_exgraphics_folder_path;
		const Configuration& config;

		std::unordered_set<ResourceDependency> determineDependencies() override;

	public:
		void insert() override;

		ExGraphics(const Configuration& config);
	};
}
