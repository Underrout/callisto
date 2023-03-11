#pragma once

#include <string>
#include <filesystem>

#include <fmt/format.h>

#include <boost/process/system.hpp>

#include "../insertable.h"
#include "../not_found_exception.h"
#include "rom_insertable.h"

namespace bp = boost::process;
namespace fs = std::filesystem;

namespace stardust {
	class LunarMagicInsertable : public RomInsertable {
	protected:
		const fs::path lunar_magic_path;

		LunarMagicInsertable(const fs::path& lunar_magic_path, const fs::path& temporary_rom_path);

		template<typename... Args>
		int callLunarMagic(Args... args) {
			return bp::system(lunar_magic_path.string(), args...);
		}

		std::unordered_set<Dependency> determineDependencies() override;
	};
}
