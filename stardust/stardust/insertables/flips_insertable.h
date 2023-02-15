#pragma once

#include <string>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <fmt/format.h>

#include <boost/process/system.hpp>

#include "lunar_magic_insertable.h"
#include "../not_found_exception.h"
#include "rom_insertable.h"

namespace bp = boost::process;
namespace fs = std::filesystem;

namespace stardust {
	class FlipsInsertable : public LunarMagicInsertable {
	protected:
		const fs::path flips_path;
		const fs::path clean_rom_path;

		FlipsInsertable(const fs::path& flips_path, const fs::path& clean_rom_path, 
			const fs::path& lunar_magic_path, const fs::path& temporary_rom_path);

		int bpsToRom(const fs::path& bps_path, const fs::path& output_rom_path) const;
	};
}
