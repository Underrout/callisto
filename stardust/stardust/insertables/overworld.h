#pragma once

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "flips_insertable.h"
#include "../not_found_exception.h"
#include "../insertion_exception.h"

namespace stardust {
	class Overworld : public FlipsInsertable {
	private:
		inline std::string getTemporaryPatchedRomPostfix() const {
			return "_overworld";
		}

		inline std::string getLunarMagicFlag() const {
			return "-TransferOverworld";
		}

		inline std::string getResourceName() const {
			return "Overworld";
		}

	public:
		Overworld(const fs::path& flips_path, const fs::path& clean_rom_path,
			const fs::path& lunar_magic_path, const fs::path& temporary_rom_path,
			const fs::path& overworld_bps_patch_path) 
			: FlipsInsertable(flips_path, clean_rom_path, lunar_magic_path, temporary_rom_path, overworld_bps_patch_path) 
		{
			checkPatchExists();
		}
	};
}
