#pragma once

#include <filesystem>

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_insertable.h"
#include "../insertion_exception.h"
#include "../not_found_exception.h"

namespace fs = std::filesystem;

namespace stardust {
	class TitleMoves : public LunarMagicInsertable {
	protected:
		const fs::path title_moves_path;

		std::unordered_set<Dependency> determineDependencies() override;

	public:
		void insert() override;

		TitleMoves(const fs::path& lunar_magic_path, const fs::path& temporary_rom_path, const fs::path& title_moves_path);
	};
}
