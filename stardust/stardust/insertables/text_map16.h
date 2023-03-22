#pragma once

#include <filesystem>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

#include "../human_map16/human_readable_map16.h"
#include "../human_map16/human_map16_exception.h"

#include "../configuration/configuration.h"

namespace fs = std::filesystem;

namespace stardust {
	class TextMap16 : public LunarMagicInsertable {
	protected:
		const fs::path map16_folder_path;

		fs::path getTemporaryMap16FilePath() const;
		fs::path generateTemporaryMap16File() const;
		void deleteTemporaryMap16File() const;

		std::unordered_set<ResourceDependency> determineDependencies() override;

	public:
		void insert() override;

		TextMap16(const Configuration& config);
	};
}
