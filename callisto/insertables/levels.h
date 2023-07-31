#pragma once

#include <filesystem>
#include <optional>
#include <functional>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

#include "../configuration/configuration.h"
#include "../dependency/policy.h"

namespace fs = std::filesystem;

namespace callisto {
	class Levels : public LunarMagicInsertable {
	protected:
		static constexpr auto MWL_DATA_POINTER_TABLE_POINTER_OFFSET{ 0x4 };

		const fs::path levels_folder;
		std::optional<std::string> import_flag;

		static size_t readBytesAt(std::fstream& stream, size_t offset, size_t count);

		static std::optional<int> getExternalLevelNumber(const fs::path& mwl_path);
		static void setInternalLevelNumber(const fs::path& mwl_path, int level_number);

		std::unordered_set<ResourceDependency> determineDependencies() override;

	public:
		static std::optional<int> getInternalLevelNumber(const fs::path& mwl_path);
		static void normalizeMwls(const fs::path& levels_folder_path, bool allow_user_input);

		void insert() override;

		Levels(const Configuration& config);
	};
}
