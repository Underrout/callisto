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
	class Graphics : public LunarMagicInsertable {
	protected:
		static const char* GRAPHICS_FOLDER_NAME;

		const fs::path project_graphics_folder_path;
		const fs::path temporary_graphics_folder_path;

		void createTemporaryGraphicsFolder() const;
		void deleteTemporaryGraphicsFolder() const;

		std::unordered_set<ResourceDependency> determineDependencies() override;

	public:
		void insert() override;

		Graphics(const Configuration& config);
	};
}
