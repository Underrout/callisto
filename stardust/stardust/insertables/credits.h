#pragma once

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "flips_insertable.h"
#include "../not_found_exception.h"
#include "../insertion_exception.h"

namespace stardust {
	class Credits : public FlipsInsertable {
	private:
		inline std::string getTemporaryPatchedRomPostfix() const {
			return "_credits";
		}

		inline std::string getLunarMagicFlag() const {
			return "-TransferCredits";
		}

		inline std::string getResourceName() const {
			return "Credits";
		}

	public:
		Credits(const fs::path& flips_path, const fs::path& clean_rom_path,
			const fs::path& lunar_magic_path, const fs::path& temporary_rom_path,
			const fs::path& credits_bps_patch)
			: FlipsInsertable(flips_path, clean_rom_path, lunar_magic_path, temporary_rom_path, credits_bps_patch)
		{
			checkPatchExists();
		}
	};
}
