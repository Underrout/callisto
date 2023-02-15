#pragma once

#include <fmt/core.h>

#include <spdlog/spdlog.h>

#include "flips_insertable.h"
#include "../not_found_exception.h"
#include "../insertion_exception.h"

namespace stardust {
	class Overworld : public FlipsInsertable {
	private:
		static const char* OVERWORLD_ROM_POSTFIX;

		const fs::path overworld_bps_path;

		fs::path getTemporaryOverworldRomPath() const;
		fs::path createTemporaryOverworldRom() const;
		void deleteTemporaryOverworldRom(const fs::path& temporary_ow_rom_path) const;

	public:
		void insert() override;

		Overworld(const fs::path& lunar_magic_path, const fs::path& flips_path, 
			const fs::path& clean_rom_path, const fs::path& temporary_rom_path,
			const fs::path& overworld_bps_path);
	};
}
