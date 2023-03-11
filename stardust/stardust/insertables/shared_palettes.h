#pragma once

#include "../insertable.h"

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "lunar_magic_insertable.h"
#include "../insertion_exception.h"

namespace fs = std::filesystem;

namespace stardust {
	class SharedPalettes : public LunarMagicInsertable {
	protected:
		const fs::path shared_palettes_path;

		std::unordered_set<Dependency> determineDependencies() override;

	public:
		void insert() override;

		SharedPalettes(const fs::path& lunar_magic_path, const fs::path& temporary_rom_path, const fs::path& shared_palettes_path);
	};
}
