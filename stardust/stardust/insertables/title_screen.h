#pragma once

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "flips_insertable.h"
#include "../not_found_exception.h"
#include "../insertion_exception.h"

namespace stardust {
	class TitleScreen : public FlipsInsertable {
	private:
		inline std::string getTemporaryPatchedRomPostfix() const {
			return "_title_screen";
		}

		inline std::string getLunarMagicFlag() const {
			return "-TransferTitleScreen";
		}

		inline std::string getResourceName() const {
			return "Title Screen";
		}

	public:
		TitleScreen(const fs::path& flips_path, const fs::path& clean_rom_path,
			const fs::path& lunar_magic_path, const fs::path& temporary_rom_path,
			const fs::path& title_screen_bps_patch_path)
			: FlipsInsertable(flips_path, clean_rom_path, lunar_magic_path, temporary_rom_path, title_screen_bps_patch_path) 
		{
			checkPatchExists();
		}
	};
}
