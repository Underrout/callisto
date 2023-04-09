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
	class ExGraphics : public LunarMagicInsertable {
	protected:
		static const char* EXGRAPHICS_FOLDER_NAME;

		const fs::path project_exgraphics_folder_path;
		const fs::path temporary_exgraphics_folder_path;

		void createTemporaryExGraphicsFolder() const;
		void deleteTemporaryExGraphicsFolder() const;

		std::unordered_set<ResourceDependency> determineDependencies() override;

	public:
		void insert() override;

		ExGraphics(const Configuration& config);
	};
}
