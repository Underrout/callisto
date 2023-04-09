#pragma once

#include <filesystem>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

#include "../configuration/configuration.h"

namespace fs = std::filesystem;

namespace stardust {
	class TitleMoves : public LunarMagicInsertable {
	protected:
		const fs::path title_moves_path;

		std::unordered_set<ResourceDependency> determineDependencies() override;

	public:
		void insert() override;

		TitleMoves(const Configuration& config);
	};
}
