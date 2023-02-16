#pragma once

#include <string>
#include <filesystem>

#include <fmt/format.h>

#include "../insertable.h"
#include "../not_found_exception.h"

namespace bp = boost::process;
namespace fs = std::filesystem; 

namespace stardust {
	class RomInsertable : public Insertable {
	protected:
		const fs::path temporary_rom_path;

		RomInsertable(const fs::path& temporary_rom_path) : temporary_rom_path(temporary_rom_path) {
			if (!fs::exists(temporary_rom_path)) {
				throw RomNotFoundException(fmt::format(
					"Temporary ROM not found at {}",
					temporary_rom_path.string()
				));
			}
		}
	};
}
